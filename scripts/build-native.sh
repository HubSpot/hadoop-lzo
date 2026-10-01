#!/usr/bin/env bash
#
# Builds libgplcompression by compiling the JNI bindings and the full LZO
# library sources into a single shared object, and drops it where the JAR
# expects it:
#
#   build/native-resources/native/<Java-os.name>-<Java-os.arch>-64/lib/libgplcompression.{so,dylib}
#
# The layout and library name match what GPLNativeCodeLoader unpacks at runtime.
# LZO is compiled in directly (no separate archive, no configure/make/libtool),
# so the result is self-contained and depends only on the system C library.
#
# Builds for the host architecture by default. On Linux, set TARGET_ARCH to
# cross-compile for another architecture with clang. This needs the matching
# cross toolchain installed (gcc-<triple> and libc6-dev-<arch>-cross), which
# provide the sysroot, crt objects, libgcc, and the GNU linker.
# Supported TARGET_ARCH values: amd64, aarch64, ppc64, ppc64le, s390x, riscv64
#
# Environment:
#   JAVA_HOME    required, provides jni.h
#   TARGET_ARCH  Linux cross-compile target (default: host architecture)

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

: "${JAVA_HOME:?JAVA_HOME must be set}"
HEADERS_DIR="build/native-headers"
OUTPUT_ROOT="build/native-resources"
LZO_TARBALL="provided/lzo-2.10.tar.gz"

if [ ! -d "$HEADERS_DIR" ]; then
  echo "JNI headers not found in $HEADERS_DIR. Run './gradlew compileJava' first." >&2
  exit 1
fi

uname_s="$(uname -s)"
uname_m="$(uname -m)"

case "$uname_s" in
  Linux)
    os_name="Linux"; jni_os="linux"; lib_ext="so" ;;
  Darwin)
    os_name="Mac_OS_X"; jni_os="darwin"; lib_ext="dylib" ;;
  *)
    echo "Unsupported OS: $uname_s" >&2; exit 1 ;;
esac

# Normalize the host machine arch to the value Java reports in os.arch.
case "$uname_s:$uname_m" in
  Linux:x86_64)                host_arch="amd64" ;;
  Linux:aarch64|Linux:arm64)   host_arch="aarch64" ;;
  Darwin:x86_64)               host_arch="x86_64" ;;
  Darwin:arm64|Darwin:aarch64) host_arch="aarch64" ;;
  *) echo "Unsupported host arch: $uname_m on $uname_s" >&2; exit 1 ;;
esac

# The architecture to build for, defaulting to the host. On Linux a differing
# TARGET_ARCH triggers a clang cross-compile against that arch's GNU sysroot.
target_arch="${TARGET_ARCH:-$host_arch}"

# Resolve the target to its os.arch directory name and (on Linux) the clang
# target triple whose sysroot lives at /usr/<triple>.
triple=""
if [ "$os_name" = "Linux" ]; then
  case "$target_arch" in
    amd64|x86_64)  java_arch="amd64";       triple="x86_64-linux-gnu" ;;
    aarch64|arm64) java_arch="aarch64";     triple="aarch64-linux-gnu" ;;
    ppc64)         java_arch="ppc64";       triple="powerpc64-linux-gnu" ;;
    ppc64le)       java_arch="ppc64le";     triple="powerpc64le-linux-gnu" ;;
    s390x)         java_arch="s390x";       triple="s390x-linux-gnu" ;;
    riscv64)       java_arch="riscv64";     triple="riscv64-linux-gnu" ;;
    *) echo "Unsupported Linux target arch: $target_arch" >&2; exit 1 ;;
  esac
else
  case "$target_arch" in
    x86_64|amd64)  java_arch="x86_64" ;;
    aarch64|arm64) java_arch="aarch64" ;;
    *) echo "Unsupported $os_name target arch: $target_arch" >&2; exit 1 ;;
  esac
fi

PLATFORM_DIR="${os_name}-${java_arch}-64"

BUILD_DIR="build/native-build"
OUT_LIB_DIR="$OUTPUT_ROOT/native/$PLATFORM_DIR/lib"

echo "==> Building libgplcompression for $PLATFORM_DIR"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR" "$OUT_LIB_DIR"

# Unpack the LZO sources; they are compiled straight into the library below.
tar xf "$LZO_TARBALL" -C "$BUILD_DIR"
LZO_SRC="$(find "$BUILD_DIR" -maxdepth 1 -type d -name 'lzo-*' | head -n1)"

# JNI bindings plus every LZO compressor/decompressor source file. LZO uses its
# own portable feature detection unless LZO_HAVE_CONFIG_H is set, so we simply
# don't define it and skip its autoconf step entirely.
SRCS=(
  src/main/native/impl/lzo/LzoCompressor.c
  src/main/native/impl/lzo/LzoDecompressor.c
)
SRCS+=("$LZO_SRC"/src/*.c)

INCLUDES=(
  "-I${JAVA_HOME}/include"
  "-I${JAVA_HOME}/include/${jni_os}"
  "-I${HEADERS_DIR}"
  "-Isrc/main/native/impl"
  "-I${LZO_SRC}/include"
  "-I${LZO_SRC}/src"
)
OUT="$OUT_LIB_DIR/libgplcompression.$lib_ext"

echo "==> Compiling $((${#SRCS[@]})) source files into $OUT"
if [ "$os_name" = "Darwin" ]; then
  clang -dynamiclib -fPIC -O2 "${INCLUDES[@]}" "${SRCS[@]}" -o "$OUT"
elif [ "$target_arch" = "$host_arch" ]; then
  # Native Linux build: clang compiles and links in one step.
  clang -shared -fPIC -O2 "${INCLUDES[@]}" "${SRCS[@]}" -o "$OUT"
else
  # Linux cross build: clang compiles every source for the target triple (using
  # that arch's sysroot to find its headers), then the target's GNU gcc driver
  # links the shared object. Letting gcc link keeps clang out of the final link,
  # where passing --sysroot makes ld re-prepend the sysroot to the absolute paths
  # in the cross libc's linker script and fail to find libc. gcc resolves the crt
  # objects, libc, libgcc, and the GNU linker itself, which also sidesteps lld's
  # missing s390x backend. JNI headers are arch-independent, so the host
  # JAVA_HOME include/linux dir is reused as-is.
  echo "==> Cross-compiling for $triple (sysroot /usr/$triple)"
  OBJ_DIR="$BUILD_DIR/obj"
  mkdir -p "$OBJ_DIR"
  OBJS=()
  i=0
  for src in "${SRCS[@]}"; do
    obj="$OBJ_DIR/$i-$(basename "$src" .c).o"
    clang --target="$triple" --sysroot="/usr/$triple" -fPIC -O2 \
      "${INCLUDES[@]}" -c "$src" -o "$obj"
    OBJS+=("$obj")
    i=$((i + 1))
  done
  "/usr/bin/${triple}-gcc" -shared -fPIC "${OBJS[@]}" -o "$OUT"
fi

echo "==> Done: $OUT"
ls -l "$OUT"
