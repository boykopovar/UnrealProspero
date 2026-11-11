import argparse
from pathlib import Path
import subprocess


def run(arguments):
    subprocess.run([str(argument) for argument in arguments], check=True, timeout=120)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dxc", type=Path, required=True)
    parser.add_argument("--adk", type=Path, required=True)
    parser.add_argument("--reflect", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    options = parser.parse_args()
    dxc = options.dxc.resolve(strict=True)
    adk = options.adk.resolve(strict=True)
    reflect = options.reflect.resolve(strict=True)
    output = options.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    shaders = Path(__file__).resolve().parent / "shaders"
    stages = (
        ("vertex", "vs_6_2", "Fullscreen.vert.hlsl", []),
        ("fragment", "ps_6_2", "Color.frag.hlsl", []),
        ("compute", "cs_6_2", "Write.comp.hlsl", ["0,0,storage-buffer,1,0,16"]),
        ("fragment", "ps_6_2", "Texture.frag.hlsl", ["0,2,sampled-image,2,0,32", "0,7,sampler,1,64,16", "1,3,uniform-buffer,1,0,16"]),
    )
    for stage, profile, source, expected in stages:
        spirv = output / (source + ".spv")
        run([dxc, "-spirv", "-fspv-target-env=vulkan1.1", "-WX", "-T", profile, "-E", "main", shaders / source, "-Fo", spirv])
        data = spirv.read_bytes()
        if len(data) < 20 or len(data) % 4 or data[:4] != b"\x03\x02\x23\x07":
            raise RuntimeError(f"Invalid SPIR-V output: {spirv}")
        reflected = subprocess.run([str(reflect), str(spirv)], check=True, capture_output=True, text=True, timeout=120).stdout.splitlines()
        if reflected != expected:
            raise RuntimeError(f"Unexpected reflection for {source}: {reflected}")
        descriptors = [argument for binding in reflected for argument in ("--descriptor", binding)]
        for target in ("windows", "linux", "ps5"):
            directory = output / target / source
            directory.mkdir(parents=True)
            header = directory / "header.bin"
            code = directory / "code.bin"
            metadata = directory / "bindings.adk"
            run([adk, "spirv", "--input", spirv, "--stage", stage, "--target", target, "--header", header, "--code", code, "--metadata", metadata, *descriptors])
            for path in (header, code, metadata):
                if not path.read_bytes():
                    raise RuntimeError(f"Empty shader output: {path}")
            print(f"Compiled {target}/{source}", flush=True)


if __name__ == "__main__":
    main()
