import argparse
from pathlib import Path
import subprocess


def run(arguments, cwd=None):
    subprocess.run([str(argument) for argument in arguments], cwd=cwd, check=True, timeout=120)


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
        ("compute", "cs_6_2", "TypedBuffer.comp.hlsl", ["0,0,uniform-texel-buffer,1,0,16", "0,1,storage-texel-buffer,1,16,16"]),
        ("compute", "cs_6_2", "VolumeWrite.comp.hlsl", ["0,0,storage-image,1,0,32", "0,1,storage-buffer,1,32,16"]),
        ("compute", "cs_6_2", "VolumeLoad.comp.hlsl", ["0,0,sampled-image,1,0,32", "0,1,storage-buffer,1,32,16"]),
        ("compute", "cs_6_2", "TextureWrite.comp.hlsl", ["0,0,storage-image,1,0,32", "0,1,storage-buffer,1,32,16"]),
        ("compute", "cs_6_2", "TextureSample.comp.hlsl", ["0,0,sampled-image,1,0,32", "0,1,sampler,1,32,16", "0,2,storage-buffer,1,48,16"]),
        ("compute", "cs_6_2", "TextureLoad.comp.hlsl", ["0,0,sampled-image,1,0,32", "0,1,storage-buffer,1,32,16"]),
        ("compute", "cs_6_2", "TextureMipLoad.comp.hlsl", ["0,0,sampled-image,1,0,32", "0,1,storage-buffer,1,32,16"]),
        ("compute", "cs_6_2", "TextureView.comp.hlsl", ["0,0,sampled-image,1,0,32", "0,1,storage-buffer,1,32,16"]),
        ("vertex", "vs_6_2", "Fullscreen.vert.hlsl", []),
        ("vertex", "vs_6_2", "VertexStreams.vert.hlsl", []),
        ("vertex", "vs_6_2", "VertexTypes.vert.hlsl", []),
        ("fragment", "ps_6_2", "Color.frag.hlsl", []),
        ("compute", "cs_6_2", "Constants.comp.hlsl", ["0,0,storage-buffer,1,0,16", "1,0,uniform-buffer,1,0,16"]),
        ("compute", "cs_6_2", "Write.comp.hlsl", ["0,0,storage-buffer,1,0,16"]),
        ("fragment", "ps_6_2", "Texture.frag.hlsl", ["0,2,sampled-image,2,0,32", "0,7,sampler,1,64,16", "1,3,uniform-buffer,1,0,16"]),
        ("fragment", "ps_6_2", "Globals.frag.hlsl", ["0,0,uniform-buffer,1,0,16"]),
        ("compute", "cs_6_2", "ReadWrite.comp.hlsl", ["0,0,storage-buffer,1,0,16", "0,1,storage-buffer,1,16,16"]),
        ("fragment", "ps_6_2", "LargeUniform.frag.hlsl", ["0,0,uniform-buffer,1,0,16"]),
    )
    details = {
        "TypedBuffer.comp.hlsl": ['"inputValues" 2 0', '"outputValues" 3 0'],
        "VolumeWrite.comp.hlsl": ['"outputTexture" 3 0', '"outputValues" 3 0'],
        "VolumeLoad.comp.hlsl": ['"inputTexture" 2 0', '"outputValues" 3 0'],
        "TextureWrite.comp.hlsl": ['"outputTexture" 3 0', '"outputValues" 3 0'],
        "TextureSample.comp.hlsl": ['"inputTexture" 2 0', '"textureSampler" 1 0', '"outputValues" 3 0'],
        "TextureView.comp.hlsl": ['"inputTexture" 2 0', '"outputValues" 3 0'],
        "TextureLoad.comp.hlsl": ['"inputTexture" 2 0', '"outputValues" 3 0'],
        "TextureMipLoad.comp.hlsl": ['"inputTexture" 2 0', '"outputValues" 3 0'],
        "Fullscreen.vert.hlsl": [],
        "VertexStreams.vert.hlsl": [],
        "VertexTypes.vert.hlsl": [],
        "Color.frag.hlsl": [],
        "Constants.comp.hlsl": ['"outputValues" 3 0', '"Constants" 0 16', '"multiplier" 0 4', '"addend" 4 4'],
        "Write.comp.hlsl": ['"outputValues" 3 0'],
        "Texture.frag.hlsl": ['"textures" 2 0', '"textureSampler" 1 0', '"Constants" 0 16', '"tint" 0 16'],
        "Globals.frag.hlsl": ['"$Globals" 0 32', '"tint" 0 16', '"intensity" 16 4'],
        "ReadWrite.comp.hlsl": ['"inputValues" 2 0', '"outputValues" 3 0'],
        "LargeUniform.frag.hlsl": ['"Constants" 0 65536', '"values" 0 65536'],
    }
    for stage, profile, source, expected in stages:
        spirv = output / (source + ".spv")
        run([dxc, "-spirv", "-fspv-target-env=vulkan1.1", "-fspv-use-legacy-buffer-matrix-order", "-WX", "-T", profile, "-E", "main", shaders / source, "-Fo", spirv])
        data = spirv.read_bytes()
        if len(data) < 20 or len(data) % 4 or data[:4] != b"\x03\x02\x23\x07":
            raise RuntimeError(f"Invalid SPIR-V output: {spirv}")
        reflection = subprocess.run([str(reflect), str(spirv)], check=True, capture_output=True, text=True, timeout=120)
        reflected = reflection.stdout.splitlines()
        if reflected != expected:
            raise RuntimeError(f"Unexpected reflection for {source}: {reflected}")
        if reflection.stderr.splitlines() != details[source]:
            raise RuntimeError(f"Unexpected resource details for {source}: {reflection.stderr}")
        inputs = subprocess.run([str(reflect), str(spirv), "--vertex-inputs"], capture_output=True, text=True, timeout=120)
        if stage == "vertex":
            expected_inputs = {"Fullscreen.vert.hlsl": [], "VertexStreams.vert.hlsl": ["0,3,0", "1,4,0"], "VertexTypes.vert.hlsl": ["0,2,0", "1,2,1", "2,4,2"]}[source]
            if inputs.returncode or inputs.stdout.splitlines() != expected_inputs:
                raise RuntimeError(f"Unexpected vertex inputs for {source}: {inputs.stdout} {inputs.stderr}")
        elif inputs.returncode == 0:
            raise RuntimeError(f"Non-vertex entry point accepted: {source}")
        descriptors = [argument for binding in reflected for argument in ("--descriptor", binding)]
        attributes = []
        if source == "VertexStreams.vert.hlsl":
            attributes = ["--vertex-attribute", "0,3,r32g32b32_float,0,12,4,0", "--vertex-attribute", "1,7,r32g32b32a32_float,0,16,4,0"]
        if source == "VertexTypes.vert.hlsl":
            attributes = ["--vertex-attribute", "0,0,r32g32_float,0,8,4,0", "--vertex-attribute", "1,1,r32g32_sint,0,8,4,0", "--vertex-attribute", "2,2,r32g32b32a32_uint,0,16,4,0"]
        for target in ("windows", "linux", "ps5"):
            directory = output / target / source
            directory.mkdir(parents=True)
            header = directory / "header.bin"
            code = directory / "code.bin"
            metadata = directory / "bindings.adk"
            (directory / "shader.spv").write_bytes(data)
            run([adk, "spirv", "--input", "shader.spv", "--header", "header.bin", "--code", "code.bin", "--metadata", "bindings.adk", "--stage", stage, "--target", target, "--entry", "main", "--omit-implicit-primitive-id", "true", *descriptors, *attributes], cwd=directory)
            for path in (header, code, metadata):
                if not path.read_bytes():
                    raise RuntimeError(f"Empty shader output: {path}")
            print(f"Compiled {target}/{source}", flush=True)


if __name__ == "__main__":
    main()
