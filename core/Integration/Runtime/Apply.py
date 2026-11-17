import argparse
from pathlib import Path


def apply(engine):
    runtime = engine / "Source/Runtime"
    ticker_sites = {
        "Dataflow/Core/Private/Dataflow/DataflowContextEvaluator.cpp": 2,
        "Dataflow/Core/Private/Dataflow/DataflowTaskGraph.cpp": 1,
        "Online/BuildPatchServices/Private/BuildPatchGeneration.cpp": 1,
        "Online/BuildPatchServices/Private/Generation/ChunkHarvester.cpp": 1,
        "Online/BuildPatchServices/Private/Generation/ChunkDeltaOptimiser.cpp": 1,
        "Online/Experimental/BandwidthMeasurementTool/Private/BandwidthMeasurementTool.cpp": 2,
        "Online/Experimental/DownlinkBandwidthManager/Private/DownlinkBandwidthManager.cpp": 1,
    }
    old_ticker = b"FTSTicker::GetCoreTicker().RemoveTicker("
    new_ticker = b"FTSTicker::RemoveTicker("
    changes = {}
    for name, count in ticker_sites.items():
        path = runtime / name
        data = path.read_bytes()
        if data.count(old_ticker) == count:
            changes[path] = data.replace(old_ticker, new_ticker)
        elif old_ticker in data or data.count(new_ticker) < count:
            raise RuntimeError(f"Unexpected ticker calls in {path}")

    path = runtime / "Online/Experimental/DownlinkBandwidthManager/Private/DownlinkBandwidthManager.cpp"
    data = changes.get(path, path.read_bytes())
    newline = b"\r\n" if b"\r\n" in data else b"\n"
    old_seed = newline.join((b"\tstd::random_device Device;", b"\tstd::mt19937 Generator(Device());"))
    new_seed = newline.join((
        b"\tconst FGuid RandomSeed = FGuid::NewGuid();",
        b"\tstd::seed_seq Seed{RandomSeed.A, RandomSeed.B, RandomSeed.C, RandomSeed.D};",
        b"\tstd::mt19937 Generator(Seed);",
    ))
    guid_header = b'#include "Misc/Guid.h"'
    if data.count(old_seed) == 1 and data.count(b"#include <random>") == 1:
        data = data.replace(old_seed, new_seed)
        if guid_header not in data:
            data = data.replace(b"#include <random>", guid_header + newline + b"#include <random>")
        changes[path] = data
    elif data.count(new_seed) != 1 or guid_header not in data or old_seed in data:
        raise RuntimeError(f"Unexpected random seed code in {path}")

    for path, data in changes.items():
        path.write_bytes(data)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("engine", type=Path)
    apply(parser.parse_args().engine)
