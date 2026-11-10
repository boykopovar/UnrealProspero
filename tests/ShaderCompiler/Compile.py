import argparse
from pathlib import Path
import subprocess


def run(arguments):
    subprocess.run([str(argument) for argument in arguments], check=True, timeout=120)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dxc", type=Path, required=True)
    parser.add_argument("--adk", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    options = parser.parse_args()
    dxc = options.dxc.resolve(strict=True)
    adk = options.adk.resolve(strict=True)
    output = options.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    shaders = Path(__file__).resolve().parent / "shaders"
    stages = (
        ("vertex", "vs_6_2", "Fullscreen.vert.hlsl", []),
        ("fragment", "ps_6_2", "Color.frag.hlsl", []),
        ("compute", "cs_6_2", "Write.comp.hlsl", ["--descriptor", "0,0,storage-buffer,1,0,16"]),
    )
    for stage, profile, source, descriptors in stages:
        spirv = output / (stage + ".spv")
        run([dxc, "-spirv", "-fspv-target-env=vulkan1.1", "-WX", "-T", profile, "-E", "main", shaders / source, "-Fo", spirv])
        data = spirv.read_bytes()
        if len(data) < 20 or len(data) % 4 or data[:4] != b"\x03\x02\x23\x07":
            raise RuntimeError(f"Invalid SPIR-V output: {spirv}")
        for target in ("windows", "linux", "ps5"):
            directory = output / target / stage
            directory.mkdir(parents=True)
            header = directory / "header.bin"
            code = directory / "code.bin"
            metadata = directory / "bindings.adk"
            run([adk, "spirv", "--input", spirv, "--stage", stage, "--target", target, "--header", header, "--code", code, "--metadata", metadata, *descriptors])
            for path in (header, code, metadata):
                if not path.read_bytes():
                    raise RuntimeError(f"Empty shader output: {path}")
            print(f"Compiled {target}/{stage}", flush=True)


if __name__ == "__main__":
    main()
