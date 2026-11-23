#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 6 || $# -gt 7 ]]; then
    printf '%s\n' 'Usage: Build.sh source build install sysroot anyps5-headers anycxx [jobs]' >&2
    exit 2
fi

script_directory=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
source_directory=$(cd "$1" && pwd -P)
mkdir -p "$2" "$3"
build_directory=$(cd "$2" && pwd -P)
installation=$(cd "$3" && pwd -P)
sysroot=$(cd "$4" && pwd -P)
headers=$(cd "$5" && pwd -P)
anycxx=$(cd "$6" && pwd -P)
jobs=${7:-2}

if [[ ! -f "$source_directory/Configure" || ! -d "$sysroot/usr/include/x86_64-linux-gnu" || ! -f "$headers/SceTypes.hpp" || ! -f "$anycxx/include/c++/v1/__external_threading" ]]; then
    printf '%s\n' 'Missing OpenSSL source or PS5 toolchain headers' >&2
    exit 1
fi
if [[ -e "$build_directory/Makefile" || "$build_directory" == "$source_directory" || "$build_directory" == "$installation" ]]; then
    printf '%s\n' 'OpenSSL requires a separate unconfigured build directory' >&2
    exit 1
fi
if command -v cygpath >/dev/null 2>&1; then
    sysroot=$(cygpath -m "$sysroot")
fi

python -B "$script_directory/Prepare.py" "$source_directory"
cd "$build_directory"
target_flags=(--target=x86_64-unknown-linux-gnu "--sysroot=$sysroot" "-isystem$sysroot/usr/include/x86_64-linux-gnu" -fPIC -fno-stack-protector -fno-builtin -D__errno_location=__error)
c_compiler=(clang)
cxx_compiler=(clang++)
if [[ -n ${UNREALPROSPERO_CCACHE:-} ]]; then
    c_compiler=("$UNREALPROSPERO_CCACHE" clang)
    cxx_compiler=("$UNREALPROSPERO_CCACHE" clang++)
fi
printf -v openssl_compiler '%q ' "${c_compiler[@]}"
CC="$openssl_compiler" AR=llvm-ar RANLIB=llvm-ranlib perl "$source_directory/Configure" linux-x86_64 \
    no-shared no-apps no-tests no-engine no-dso no-module no-asm no-sock no-stdio \
    no-secure-memory no-async no-thread-pool no-argon2 no-ui-console no-autoload-config \
    --with-rand-seed=getrandom "${target_flags[@]}"
make -j "$jobs" build_libs

for bridge in Directory Entropy Environment FileStatus Once Time; do
    "${cxx_compiler[@]}" "${target_flags[@]}" -nostdinc++ -isystem "$anycxx/include/c++/v1" \
        -isystem "$anycxx/include/anyps5" -isystem "$headers" -std=c++20 \
        -Wall -Wextra -Wpedantic -Werror -c "$script_directory/src/$bridge.cpp" -o "$bridge.o"
done
llvm-objcopy --redefine-sym=__isoc99_sscanf=sscanf \
    --redefine-sym=__xpg_strerror_r=strerror_r_nid_postfix \
    --redefine-sym=stat=UnrealProsperoOpenSslStat --redefine-sym=lstat=UnrealProsperoOpenSslLstat \
    --redefine-sym=opendir=UnrealProsperoOpenSslOpenDirectory --redefine-sym=readdir=UnrealProsperoOpenSslReadDirectory \
    --redefine-sym=gmtime_r=UnrealProsperoOpenSslGmtime \
    --redefine-sym=closedir=UnrealProsperoOpenSslCloseDirectory libcrypto.a libcrypto-ps5.a
llvm-ar rcs libcrypto-ps5.a Directory.o Entropy.o Environment.o FileStatus.o Once.o Time.o
llvm-objcopy --redefine-sym=stat=UnrealProsperoOpenSslStat libssl.a libssl-ps5.a

mkdir -p "$installation/lib" "$installation/include/openssl"
cp libssl-ps5.a "$installation/lib/libssl.a"
cp libcrypto-ps5.a "$installation/lib/libcrypto.a"
cp "$source_directory/include/openssl/"*.h "$installation/include/openssl/"
cp include/openssl/*.h "$installation/include/openssl/"
