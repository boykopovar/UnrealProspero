import argparse
from pathlib import Path

from unreal_prospero.engine import require_engine
from unreal_prospero.ports import Ports
from unreal_prospero.process import require_tool


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--engine', required=True, type=Path)
    parser.add_argument('--sdk', required=True, type=Path)
    parser.add_argument('--deps', required=True, type=Path)
    parser.add_argument('--work', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--cmake', type=Path)
    parser.add_argument('--jobs', type=int, default=4)
    options = parser.parse_args()
    if options.jobs < 1:
        raise ValueError('Job count must be positive')
    output = options.output.resolve()
    if output.exists():
        raise FileExistsError(output)
    cmake = str(options.cmake.resolve(strict=True)) if options.cmake is not None else require_tool('cmake')
    ports = Ports(require_engine(options.engine), options.sdk.resolve(strict=True), options.deps.resolve(strict=True), options.work.resolve(), output, cmake, options.jobs)
    ports.third_party_ports()
    ports.network_ports()
    ports.write_manifest()


if __name__ == '__main__':
    main()
