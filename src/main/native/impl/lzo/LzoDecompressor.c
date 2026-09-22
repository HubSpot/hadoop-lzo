/*
 * This file is part of Hadoop-Gpl-Compression.
 *
 * Hadoop-Gpl-Compression is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Hadoop-Gpl-Compression is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hadoop-Gpl-Compression.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "gpl-compression.h"
#include "lzo.h"
#include <stdlib.h>

// lzo2 library version
static jint liblzo2_version = 0;

#define MSG_LEN 1024

// The lzo 'decompressors'. Each entry points directly at the statically linked
// lzo function; the name is kept for error messages. The _asm_ variants are
// implemented only in i386 assembly, which is not built here, so their pointers
// are NULL and selecting them raises UnsatisfiedLinkError (as before).
typedef struct {
  void *func;
  const char *name;
} lzo_decompressor;

// The index of each entry matches the ordinal of the corresponding
// CompressionStrategy value in LzoDecompressor.java.
static lzo_decompressor lzo_decompressors[] = {
  /** lzo1 decompressors */
  /* 0 */   {(void*)lzo1_decompress, "lzo1_decompress"},

  /** lzo1a decompressors */
  /* 1 */   {(void*)lzo1a_decompress, "lzo1a_decompress"},

  /** lzo1b decompressors */
  /* 2 */   {(void*)lzo1b_decompress, "lzo1b_decompress"},
  /* 3 */   {(void*)lzo1b_decompress_safe, "lzo1b_decompress_safe"},

  /** lzo1c decompressors */
  /* 4 */   {(void*)lzo1c_decompress, "lzo1c_decompress"},
  /* 5 */   {(void*)lzo1c_decompress_safe, "lzo1c_decompress_safe"},
  /* 6 */   {NULL, "lzo1c_decompress_asm"},
  /* 7 */   {NULL, "lzo1c_decompress_asm_safe"},

  /** lzo1f decompressors */
  /* 8 */   {(void*)lzo1f_decompress, "lzo1f_decompress"},
  /* 9 */   {(void*)lzo1f_decompress_safe, "lzo1f_decompress_safe"},
  /* 10 */  {NULL, "lzo1f_decompress_asm_fast"},
  /* 11 */  {NULL, "lzo1f_decompress_asm_fast_safe"},

  /** lzo1x decompressors */
  /* 12 */  {(void*)lzo1x_decompress, "lzo1x_decompress"},
  /* 13 */  {(void*)lzo1x_decompress_safe, "lzo1x_decompress_safe"},
  /* 14 */  {NULL, "lzo1x_decompress_asm"},
  /* 15 */  {NULL, "lzo1x_decompress_asm_safe"},
  /* 16 */  {NULL, "lzo1x_decompress_asm_fast"},
  /* 17 */  {NULL, "lzo1x_decompress_asm_fast_safe"},

  /** lzo1y decompressors */
  /* 18 */  {(void*)lzo1y_decompress, "lzo1y_decompress"},
  /* 19 */  {(void*)lzo1y_decompress_safe, "lzo1y_decompress_safe"},
  /* 20 */  {NULL, "lzo1y_decompress_asm"},
  /* 21 */  {NULL, "lzo1y_decompress_asm_safe"},
  /* 22 */  {NULL, "lzo1y_decompress_asm_fast"},
  /* 23 */  {NULL, "lzo1y_decompress_asm_fast_safe"},

  /** lzo1z decompressors */
  /* 24 */  {(void*)lzo1z_decompress, "lzo1z_decompress"},
  /* 25 */  {(void*)lzo1z_decompress_safe, "lzo1z_decompress_safe"},

  /** lzo2a decompressors */
  /* 26 */  {(void*)lzo2a_decompress, "lzo2a_decompress"},
  /* 27 */  {(void*)lzo2a_decompress_safe, "lzo2a_decompress_safe"}
};

static jfieldID LzoDecompressor_clazz;
static jfieldID LzoDecompressor_finished;
static jfieldID LzoDecompressor_compressedDirectBuf;
static jfieldID LzoDecompressor_compressedDirectBufLen;
static jfieldID LzoDecompressor_uncompressedDirectBuf;
static jfieldID LzoDecompressor_directBufferSize;
static jfieldID LzoDecompressor_lzoDecompressor;

JNIEXPORT void JNICALL
Java_com_hadoop_compression_lzo_LzoDecompressor_initIDs(
	JNIEnv *env, jclass class
	) {
  LzoDecompressor_clazz = (*env)->GetStaticFieldID(env, class, "clazz", 
                                                   "Ljava/lang/Class;");
  LzoDecompressor_finished = (*env)->GetFieldID(env, class, "finished", "Z");
  LzoDecompressor_compressedDirectBuf = (*env)->GetFieldID(env, class, 
                                                "compressedDirectBuf", 
                                                "Ljava/nio/Buffer;");
  LzoDecompressor_compressedDirectBufLen = (*env)->GetFieldID(env, class, 
                                                    "compressedDirectBufLen", "I");
  LzoDecompressor_uncompressedDirectBuf = (*env)->GetFieldID(env, class, 
                                                  "uncompressedDirectBuf", 
                                                  "Ljava/nio/Buffer;");
  LzoDecompressor_directBufferSize = (*env)->GetFieldID(env, class, 
                                              "directBufferSize", "I");
  LzoDecompressor_lzoDecompressor = (*env)->GetFieldID(env, class,
                                              "lzoDecompressor", "J");

  // record lzo library version
  liblzo2_version = (jint) lzo_version();
}

JNIEXPORT void JNICALL
Java_com_hadoop_compression_lzo_LzoDecompressor_init(
  JNIEnv *env, jobject this, jint decompressor 
  ) {
  int rv = 0;
  void *decompressor_func_ptr = lzo_decompressors[decompressor].func;

  // Initialize the lzo library
  rv = lzo_init();
  if (rv != LZO_E_OK) {
    THROW(env, "Ljava/lang/InternalError", "Could not initialize lzo library!");
    return;
  }

  if (decompressor_func_ptr == NULL) {
    THROW(env, "java/lang/UnsatisfiedLinkError",
          lzo_decompressors[decompressor].name);
    return;
  }

  // Save the decompressor-function into LzoDecompressor_lzoDecompressor
  (*env)->SetLongField(env, this, LzoDecompressor_lzoDecompressor,
                       JLONG(decompressor_func_ptr));

  return;
}

JNIEXPORT jint JNICALL
Java_com_hadoop_compression_lzo_LzoDecompressor_getLzoLibraryVersion(
    JNIEnv* env, jclass class) {
  return liblzo2_version;
}

JNIEXPORT jint JNICALL
Java_com_hadoop_compression_lzo_LzoDecompressor_decompressBytesDirect(
	JNIEnv *env, jobject this, jint decompressor
	) {
  jobject clazz = NULL;
  jobject compressed_direct_buf = NULL;
  lzo_uint compressed_direct_buf_len = 0;
  jobject uncompressed_direct_buf = NULL;
  lzo_uint uncompressed_direct_buf_len = 0;
  jlong lzo_decompressor_funcptr = 0;
  lzo_bytep uncompressed_bytes = NULL;
  lzo_bytep compressed_bytes = NULL;
  lzo_uint no_uncompressed_bytes = 0;
  lzo_decompress_t fptr = NULL;
  int rv = 0;
  char exception_msg[MSG_LEN];
  const char *lzo_decompressor_function = lzo_decompressors[decompressor].name;

	// Get members of LzoDecompressor
	clazz = (*env)->GetStaticObjectField(env, this, 
	                                             LzoDecompressor_clazz);
	compressed_direct_buf = (*env)->GetObjectField(env, this,
                                              LzoDecompressor_compressedDirectBuf);
	compressed_direct_buf_len = (*env)->GetIntField(env, this, 
                        		  							LzoDecompressor_compressedDirectBufLen);

	uncompressed_direct_buf = (*env)->GetObjectField(env, this, 
                            								  LzoDecompressor_uncompressedDirectBuf);
	uncompressed_direct_buf_len = (*env)->GetIntField(env, this,
                                                LzoDecompressor_directBufferSize);

  lzo_decompressor_funcptr = (*env)->GetLongField(env, this,
                                              LzoDecompressor_lzoDecompressor);

    // Get the input direct buffer
    LOCK_CLASS(env, clazz, "LzoDecompressor");
	uncompressed_bytes = (*env)->GetDirectBufferAddress(env, 
											                    uncompressed_direct_buf);
    UNLOCK_CLASS(env, clazz, "LzoDecompressor");
    
 	if (uncompressed_bytes == 0) {
    return (jint)0;
	}
	
    // Get the output direct buffer
    LOCK_CLASS(env, clazz, "LzoDecompressor");
	compressed_bytes = (*env)->GetDirectBufferAddress(env, 
										                    compressed_direct_buf);
    UNLOCK_CLASS(env, clazz, "LzoDecompressor");

  if (compressed_bytes == 0) {
		return (jint)0;
	}
	
	// Decompress
  no_uncompressed_bytes = uncompressed_direct_buf_len;
  fptr = (lzo_decompress_t) FUNC_PTR(lzo_decompressor_funcptr);
  rv = fptr(compressed_bytes, compressed_direct_buf_len, uncompressed_bytes,
    &no_uncompressed_bytes, NULL); 

  if (rv == LZO_E_OK) {
    // lzo decompresses all input data
    (*env)->SetIntField(env, this, LzoDecompressor_compressedDirectBufLen, 0);
  } else {
#ifdef UNIX
    snprintf(exception_msg, MSG_LEN, "%s returned: %d", 
              lzo_decompressor_function, rv);
#endif

#ifdef WINDOWS
    _snprintf_s(exception_msg, MSG_LEN, _TRUNCATE, "%s returned: %d",
      lzo_decompressor_function, rv);
#endif

    THROW(env, "java/lang/InternalError", exception_msg);
  }
  
  return (jint)no_uncompressed_bytes;
}

/**
 * vim: sw=2: ts=2: et:
 */

