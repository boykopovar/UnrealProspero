import argparse
from pathlib import Path


def prepare_entropy(source):
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


def replace_once(path, old, new):
    data = path.read_bytes()
    newline = b"\r\n" if b"\r\n" in data else b"\n"
    old = old.replace(b"\n", newline)
    new = new.replace(b"\n", newline)
    if data.count(new) == 1:
        return
    if data.count(old) != 1:
        raise RuntimeError(f"Unexpected OpenSSL thread implementation in {path}")
    path.write_bytes(data.replace(old, new))


def prepare(source):
    prepare_entropy(source)
    replace_once(source / "include/openssl/crypto.h.in", b"typedef pthread_once_t CRYPTO_ONCE;", b"typedef unsigned int CRYPTO_ONCE;")
    replace_once(source / "include/openssl/crypto.h.in", b"#define CRYPTO_ONCE_STATIC_INIT PTHREAD_ONCE_INIT", b"#define CRYPTO_ONCE_STATIC_INIT 0u")
    replace_once(
        source / "crypto/threads_pthread.c",
        b"int CRYPTO_THREAD_run_once(CRYPTO_ONCE *once, void (*init)(void))\n{\n    if (pthread_once(once, init) != 0)",
        b"extern int UnrealProsperoOpenSslRunOnce(CRYPTO_ONCE *once, void (*init)(void));\n\nint CRYPTO_THREAD_run_once(CRYPTO_ONCE *once, void (*init)(void))\n{\n    if (UnrealProsperoOpenSslRunOnce(once, init) != 0)",
    )
    for name, calls in {
        "crypto/conf/conf_mod.c": (b"isspace((unsigned char)*lstart)", b"isspace((unsigned char)*tmpend)"),
        "crypto/encode_decode/encoder_lib.c": (b"tolower((unsigned char)p[0])", b"tolower((unsigned char)p[1])"),
        "providers/implementations/storemgmt/file_store.c": (b"tolower((unsigned char)p[1])",),
    }.items():
        for call in calls:
            function, arguments = call.split(b"(", 1)
            replace_once(source / name, call, b"(" + function + b")(" + arguments)
    file_store = source / "providers/implementations/storemgmt/file_store.c"
    for condition in (b"if (!isdigit((unsigned char)*p))", b"while (isdigit((unsigned char)*p))"):
        replace_once(file_store, condition, condition.replace(b"isdigit(", b"(isdigit)("))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    prepare(parser.parse_args().source)
