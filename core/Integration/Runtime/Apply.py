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

    path = runtime / "Experimental/Chaos/Private/Chaos/PBDSuspensionConstraints.cpp"
    data = path.read_bytes()
    newline = b"\r\n" if b"\r\n" in data else b"\n"
    old_solver = newline.join((
        b"\t\tCollisionSolvers.Add(Private::FPBDCollisionSolver());",
        b"\t\tCollisionSolvers[CollisionSolvers.Num() - 1].MakeInitialized();",
    ))
    new_solver = b"\t\tCollisionSolvers.Add(Private::FPBDCollisionSolver::MakeInitialized());"
    if data.count(old_solver) == 1 and new_solver not in data:
        changes[path] = data.replace(old_solver, new_solver)
    elif old_solver in data or data.count(new_solver) != 1:
        raise RuntimeError(f"Unexpected suspension solver initialization in {path}")

    http = runtime / "Experimental/IoStore/HttpClient/Private"
    http_sites = {
        "Socket.inl": (
            (b"#if defined(IAS_HTTP_SOCKET_FORWARD)", b"#if defined(IAS_HTTP_SOCKET_FORWARD) && !defined(UNREALPROSPERO_TARGET_PS5)"),
            (b"#else\n\tstatic int32 LastSocketResult() { return errno; }",
             b"#elif defined(UNREALPROSPERO_TARGET_PS5)\n\tstatic int32 LastSocketResult() { return errno; }\n#\tdefine IsSocketResult(err) (LastSocketResult() == Prospero_##err)\n#else\n\tstatic int32 LastSocketResult() { return errno; }"),
        ),
        "ConnectionPool.inl": (
            (b"\taddrinfo* Info = nullptr;", b"#if defined(UNREALPROSPERO_TARGET_PS5)\n\tconst int32 AddressCount = ResolveProsperoHost(HostName, MakeArrayView(IpAddresses));\n#else\n\taddrinfo* Info = nullptr;"),
            (b"\n\tif (AddressCount > 0)", b"\n#endif\n\n\tif (AddressCount > 0)"),
        ),
    }
    for name, replacements in http_sites.items():
        path = http / name
        original = path.read_bytes()
        data = original
        newline = b"\r\n" if b"\r\n" in data else b"\n"
        for old_code, new_code in replacements:
            old_code = old_code.replace(b"\n", newline)
            new_code = new_code.replace(b"\n", newline)
            if data.count(new_code) == 1:
                continue
            if data.count(old_code) != 1:
                raise RuntimeError(f"Unexpected IoStore HTTP code in {path}")
            data = data.replace(old_code, new_code)
        if data != original:
            changes[path] = data

    for path, data in changes.items():
        path.write_bytes(data)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("engine", type=Path)
    apply(parser.parse_args().engine)
