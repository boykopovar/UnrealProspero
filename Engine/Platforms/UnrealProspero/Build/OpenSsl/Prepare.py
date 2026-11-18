import argparse
from pathlib import Path


def prepare(source):
    path = source / "providers/implementations/rands/seeding/rand_unix.c"
    data = path.read_bytes()
    newline = b"\r\n" if b"\r\n" in data else b"\n"
    signature = b"static ssize_t syscall_random(void *buf, size_t buflen)"
    original = newline.join((signature, b"{"))
    replacement = newline.join((
        b"extern ssize_t UnrealProsperoOpenSslRandom(void *buf, size_t buflen);",
        b"",
        signature,
        b"{",
        b"    return UnrealProsperoOpenSslRandom(buf, buflen);",
        b"}",
        b"",
    ))
    if data.count(replacement) == 1:
        return
    if data.count(original) != 1:
        raise RuntimeError(f"Unexpected OpenSSL entropy implementation in {path}")
    start = data.index(original)
    end = data.find(newline + b"#endif /* defined(OPENSSL_RAND_SEED_GETRANDOM) */", start)
    if end < 0 or not data[start:end].rstrip().endswith(b"}"):
        raise RuntimeError(f"Unexpected OpenSSL entropy implementation boundary in {path}")
    path.write_bytes(data[:start] + replacement + data[end + len(newline):])


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    prepare(parser.parse_args().source)
