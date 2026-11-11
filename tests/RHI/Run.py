import argparse
import os
from pathlib import Path
import subprocess


def run_check(executable, arguments, environment, label, log, log_path):
    result = subprocess.run([str(executable), *map(str, arguments)], cwd=executable.parent, env=environment, capture_output=True, text=True, timeout=60)
    text = result.stdout + result.stderr
    log.write(f"{label}\n{text}\nExit: {result.returncode}\n")
    log.flush()
    if result.returncode != 0 or "VUID-" in text or "was not found but was requested by env var VK_INSTANCE_LAYERS" in text:
        raise RuntimeError(f"Shader check failed for {label}; see {log_path}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", type=Path, required=True)
    parser.add_argument("--compute-check", type=Path)
    parser.add_argument("--shaders", type=Path, required=True)
    parser.add_argument("--target", choices=("windows", "linux", "ps5"))
    parser.add_argument("--log", type=Path, required=True)
    options = parser.parse_args()
    if options.compute_check and options.target not in ("windows", "linux"):
        parser.error("--compute-check requires a native --target windows or linux")
    compute = options.compute_check.resolve(strict=True) if options.compute_check else None
    executable = options.check.resolve(strict=True)
    shaders = options.shaders.resolve(strict=True)
    targets = (options.target,) if options.target else ("windows", "linux", "ps5")
    environment = os.environ.copy()
    environment.update(ANYPS5_REQUIRE_VULKAN="1", ANYPS5_NO_SHADER_CACHE="1", VK_INSTANCE_LAYERS="VK_LAYER_KHRONOS_validation")
    count = 0
    with options.log.open("w", encoding="utf-8") as log:
        for target in targets:
            headers = sorted((shaders / target).glob("*/header.bin"))
            if not headers:
                raise RuntimeError(f"No generated shaders for {target}")
            for header in headers:
                suffix = header.parent.name.split(".")[-2]
                stage = {"vert": "vertex", "frag": "fragment", "comp": "compute"}[suffix]
                code = header.with_name("code.bin").resolve(strict=True)
                metadata = header.with_name("bindings.adk").resolve(strict=True)
                run_check(executable, (header, code, stage, metadata), environment, f"{target}/{header.parent.name}", log, options.log)
                count += 1
            if compute:
                for name, operation in (("Write.comp.hlsl", "write"), ("ReadWrite.comp.hlsl", "read-write"), ("Constants.comp.hlsl", "constants"), ("TextureLoad.comp.hlsl", "texture")):
                    directory = shaders / target / name
                    arguments = [(directory / file).resolve(strict=True) for file in ("header.bin", "code.bin", "bindings.adk")]
                    run_check(compute, (*arguments, operation), environment, f"{target}/dispatch/{name}", log, options.log)
                    count += 1
    print(f"Passed {count} AGC shader checks")


if __name__ == "__main__":
    main()
