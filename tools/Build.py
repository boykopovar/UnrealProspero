import argparse
from pathlib import Path

from unreal_prospero.build import build_host, build_managed, require_dotnet
from unreal_prospero.engine import require_engine


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--engine', required=True, type=Path)
    parser.add_argument('--work', required=True, type=Path)
    parser.add_argument('--dotnet', type=Path)
    parser.add_argument('--managed-only', action='store_true')
    parser.add_argument('--no-link', action='store_true')
    parser.add_argument('--jobs', type=int, default=4)
    options = parser.parse_args()
    if options.jobs < 1:
        raise ValueError('Job count must be positive')
    engine = require_engine(options.engine)
    dotnet = require_dotnet(engine, options.dotnet)
    work = options.work.resolve()
    build_managed(engine, dotnet, work)
    if not options.managed_only:
        build_host(engine, dotnet, work, options.jobs, options.no_link)


if __name__ == '__main__':
    main()
