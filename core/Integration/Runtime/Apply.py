import argparse
import re
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

    path = runtime / "Launch/Private/Launch.cpp"
    data = path.read_bytes()
    old_launch = b"#if PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_UNIX || PLATFORM_USE_GENERIC_LAUNCH_IMPLEMENTATION"
    new_launch = old_launch + b" || PLATFORM_UNREALPROSPERO"
    if data.count(new_launch) != 1:
        if data.count(old_launch) != 1:
            raise RuntimeError(f"Unexpected generic launch condition in {path}")
        changes[path] = data.replace(old_launch, new_launch)


    sockets = runtime / "Sockets/Private/BSDSockets"
    socket_calls = {
        "SocketsBSD.cpp": {
            "shutdown": "Shutdown", "bind": "Bind", "connect": "Connect", "listen": "Listen",
            "accept": "Accept", "sendto": "SendTo", "send": "Send", "recvfrom": "ReceiveFrom",
            "recv": "Receive", "getsockname": "GetSocketName", "getpeername": "GetPeerName",
            "setsockopt": "SetSocketOption", "getsockopt": "GetSocketOption", "ioctlsocket": "SocketIoctl",
            "poll": "Poll",
        },
        "SocketSubsystemBSD.cpp": {
            "socket": "OpenSocket", "getaddrinfo": "ResolveAddress", "freeaddrinfo": "FreeAddresses",
            "getnameinfo": "ResolveName", "inet_pton": "ParseInternetAddress", "poll": "Poll",
            "gethostname": "ReadHostName",
        },
        "IPAddressBSD.cpp": {"getnameinfo": "ResolveName"},
    }
    for name, calls in socket_calls.items():
        path = sockets / name
        original = path.read_bytes()
        marker = b'#include "UnrealProspero/SocketApi.h"'
        if marker in original:
            continue
        newline = b"\r\n" if b"\r\n" in original else b"\n"
        data = original.replace(b"\r\n", b"\n")
        include = b'\n#if PLATFORM_HAS_BSD_SOCKETS || PLATFORM_HAS_BSD_IPV6_SOCKETS\n'
        if data.count(include) != 1:
            raise RuntimeError(f"Unexpected BSD socket guard in {path}")
        mappings = b"\n".join(b"#define PROSPERO_SOCKET_" + source.encode() + b" UnrealProspero::Network::" + target.encode() for source, target in calls.items())
        defaults = b"\n".join(b"#define PROSPERO_SOCKET_" + source.encode() + b" " + source.encode() for source in calls)
        undefs = b"\n".join(b"#undef PROSPERO_SOCKET_" + source.encode() for source in calls)
        for source in calls:
            pattern = rb"(?m)^([ \t]*(?!//)[^\n\"]*?)\b" + source.encode() + rb"\("
            data, count = re.subn(pattern, lambda match: match[1] + b"PROSPERO_SOCKET_" + source.encode() + b"(", data)
            if count == 0:
                raise RuntimeError(f"Missing BSD socket call {source} in {path}")
        after_includes = {
            "SocketsBSD.cpp": b'#include "BSDSockets/SocketSubsystemBSD.h"\n',
            "SocketSubsystemBSD.cpp": b"#include <errno.h>\n",
            "IPAddressBSD.cpp": include,
        }[name]
        binding = b"\n#if defined(UNREALPROSPERO_TARGET_PS5) && UNREALPROSPERO_TARGET_PS5\n" + marker + b"\n" + mappings + b"\n#else\n" + defaults + b"\n#endif\n"
        if data.count(after_includes) != 1:
            raise RuntimeError(f"Unexpected BSD socket includes in {path}")
        data = data.replace(after_includes, after_includes + binding)
        data += b"\n" + undefs + b"\n"
        data = data.replace(b"PLATFORM_HAS_BSD_SOCKET_FEATURE_POLL && USE_SOCKET_FEATURE_POLL", b"((PLATFORM_HAS_BSD_SOCKET_FEATURE_POLL && USE_SOCKET_FEATURE_POLL) || (defined(UNREALPROSPERO_TARGET_PS5) && UNREALPROSPERO_TARGET_PS5))")
        if name == "SocketsBSD.cpp":
            data = data.replace(b"#else\n\tint Flags = fcntl(Socket, F_GETFL, 0);", b"#elif defined(UNREALPROSPERO_TARGET_PS5) && UNREALPROSPERO_TARGET_PS5\n\tint Value = bIsNonBlocking;\n\treturn PROSPERO_SOCKET_ioctlsocket(Socket, FIONBIO, &Value) == 0;\n#else\n\tint Flags = fcntl(Socket, F_GETFL, 0);")
            data = data.replace(b"#ifdef IP_PKTINFO", b"#if defined(IP_PKTINFO) && !(defined(UNREALPROSPERO_TARGET_PS5) && UNREALPROSPERO_TARGET_PS5)")
            data = data.replace(b"IPPROTO_IPV6, IP_MULTICAST_LOOP,", b"IPPROTO_IPV6, IPV6_MULTICAST_LOOP,")
        changes[path] = data.replace(b"\n", newline)

    path = engine / "Shaders/Private/ShaderPrint.ush"
    data = path.read_bytes()
    old_shader_print = b"#if ES3_1_PROFILE || (COMPILER_HLSL && !COMPILER_DXC)"
    new_shader_print = old_shader_print + b" || UNREALPROSPERO_SHADER_PRINT_GLOBAL_RESOURCES"
    if data.count(new_shader_print) != 1:
        if data.count(old_shader_print) != 1:
            raise RuntimeError(f"Unexpected shader print resource selection in {path}")
        changes[path] = data.replace(old_shader_print, new_shader_print)

    for path, data in changes.items():
        path.write_bytes(data)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("engine", type=Path)
    apply(parser.parse_args().engine)
