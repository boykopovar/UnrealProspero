import argparse
from pathlib import Path
import subprocess


def run(arguments):
    subprocess.run([str(value) for value in arguments], check=True, timeout=120)


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
    sources = Path(__file__).resolve().parent / "pipelines"
    stages = (("Triangle.vert.hlsl", "vs_6_2"), ("EmitTriangle.geom.hlsl", "gs_6_2"), ("Triangle.tesc.hlsl", "hs_6_2"), ("Triangle.tese.hlsl", "ds_6_2"))
    for name, profile in stages:
        run([dxc, "-spirv", "-fspv-target-env=vulkan1.1", "-WX", "-T", profile, "-E", "main", sources / name, "-Fo", output / (name + ".spv")])
    for source, profile in ((sources.parent / "shaders/VertexStreams.vert.hlsl", "vs_6_2"), (sources / "VertexStreams.geom.hlsl", "gs_6_2")):
        run([dxc, "-spirv", "-fspv-target-env=vulkan1.1", "-WX", "-T", profile, "-E", "main", source, "-Fo", output / (source.name + ".spv")])
    invalid = output / "invalid.spv"
    invalid.write_bytes(bytes(20))
    base = [str(adk), "shader-pipeline", "--geometry", str(output / "EmitTriangle.geom.hlsl.spv")]
    cases = (
        (["--input-format", "unknown", "--vertex", str(output / "Triangle.vert.hlsl.spv")], "Pipeline input format"),
        (["--input-format", "spirv", "--vertex", str(invalid)], "Invalid SPIR-V magic"),
        (["--input-format", "spirv", "--vertex", str(output / "Triangle.vert.hlsl.spv"), "--include", str(sources)], "cannot use source include directories"),
    )
    for index, (arguments, message) in enumerate(cases):
        destination = output / f"rejected-{index}"
        result = subprocess.run(base + arguments + ["--output", str(destination)], capture_output=True, text=True, timeout=120)
        if result.returncode == 0 or message not in result.stderr or destination.exists():
            raise RuntimeError(f"Invalid pipeline input was not rejected cleanly: {result.stderr}")
    for target in ("windows", "linux", "ps5"):
        run([adk, "shader-pipeline", "--input-format", "spirv", "--vertex", output / "VertexStreams.vert.hlsl.spv", "--geometry", output / "VertexStreams.geom.hlsl.spv", "--output", output / (target + "-geometry-streams"), "--target", target, "--vertex-attribute", "0,3,r32g32b32_float,0,12,4,0", "--vertex-attribute", "1,7,r32g32b32a32_float,0,16,4,0"])
        for pipeline in ("geometry", "tessellation"):
            directory = output / (target + "-" + pipeline)
            arguments = [adk, "shader-pipeline", "--input-format", "spirv", "--vertex", output / "Triangle.vert.hlsl.spv", "--output", directory, "--target", target]
            if pipeline == "geometry":
                arguments += ["--geometry", output / "EmitTriangle.geom.hlsl.spv"]
                expected = {"geometry": 4}
            else:
                arguments += ["--control", output / "Triangle.tesc.hlsl.spv", "--evaluation", output / "Triangle.tese.hlsl.spv", "--patch-control-points", "3"]
                expected = {"control": 2, "evaluation": 3}
            run(arguments)
            for stage, number in expected.items():
                if (directory / (stage + ".header")).read_bytes()[:4] != b"1234" or not (directory / (stage + ".code")).read_bytes():
                    raise RuntimeError("Invalid AGC pipeline output")
                fields = dict(line.split("=", 1) for line in (directory / (stage + ".adk")).read_text().splitlines()[1:])
                if int(fields["source_stage"]) != number:
                    raise RuntimeError("Wrong compiled pipeline stage")
                if pipeline == "geometry" and (fields["merged_geometry"] != "1" or fields["merged_es_source_stage"] != "1"):
                    raise RuntimeError("Geometry pipeline lost its vertex stage")
            print(f"Compiled {target}/{pipeline}", flush=True)


if __name__ == "__main__":
    main()
