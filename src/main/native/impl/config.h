/* Static configuration for building libgplcompression.
 *
 * The native library statically links lzo2 and resolves its symbols from its
 * own image, so no autotools feature detection is required. This header defines
 * the POSIX features the sources assume on Linux and macOS.
 */
#ifndef GPL_COMPRESSION_CONFIG_H
#define GPL_COMPRESSION_CONFIG_H

#define HAVE_DLFCN_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_JNI_H 1
#define HAVE_LIBDL 1
#define HAVE_LZO_LZO2A_H 1
#define HAVE_MEMSET 1
#define HAVE_MKDIR 1
#define HAVE_STDBOOL_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRERROR_R 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNAME 1
#define HAVE_UNISTD_H 1
#define HAVE_WCHAR_H 1
#define HAVE__BOOL 1
#define STDC_HEADERS 1

/* Only referenced by the unused Windows code path; kept for completeness. */
#ifndef HADOOP_LZO_LIBRARY
#define HADOOP_LZO_LIBRARY "lzo2.dll"
#endif

#endif /* GPL_COMPRESSION_CONFIG_H */
