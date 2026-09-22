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

/**
 * This file includes some common utilities.
 */
 
#if !defined GPL_COMPRESSION_H
#define GPL_COMPRESSION_H

#if defined(_WIN32)
#undef UNIX
#define WINDOWS
#else
#undef WINDOWS
#define UNIX
#endif

#include "lzo/lzo.h"

/* A helper macro to 'throw' a java exception. */ 
#define THROW(env, exception_name, message) \
  { \
	jclass ecls = (*env)->FindClass(env, exception_name); \
	if (ecls) { \
	  (*env)->ThrowNew(env, ecls, message); \
	  (*env)->DeleteLocalRef(env, ecls); \
	} \
  }

/**
 * Unix definitions
 */
#ifdef UNIX
#include <jni.h>
#endif
// Unix part end


/**
 * Windows definitions
 */
#ifdef WINDOWS

/* Force using Unicode throughout the code */
#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>
#include <stdio.h>
#include <jni.h>

#define snprintf(a, b ,c, d) _snprintf_s((a), (b), _TRUNCATE, (c), (d))

#endif
// Windows part end


#define LOCK_CLASS(env, clazz, classname) \
  if ((*env)->MonitorEnter(env, clazz) != 0) { \
    char exception_msg[128]; \
    snprintf(exception_msg, 128, "Failed to lock %s", classname); \
    THROW(env, "java/lang/InternalError", exception_msg); \
  }

#define UNLOCK_CLASS(env, clazz, classname) \
  if ((*env)->MonitorExit(env, clazz) != 0) { \
    char exception_msg[128]; \
    snprintf(exception_msg, 128, "Failed to unlock %s", classname); \
    THROW(env, "java/lang/InternalError", exception_msg); \
  }

/* A helper macro to convert the java 'function-pointer' to a void*. */
#define FUNC_PTR(func_ptr) ((void*)((ptrdiff_t)(func_ptr)))

/* A helper macro to convert the void* to the java 'function-pointer'. */
#define JLONG(func_ptr) ((jlong)((ptrdiff_t)(func_ptr)))

// type of pointer to lzo function that returns version number
typedef unsigned (__LZO_CDECL *lzo_version_t)();

// type of pointer to lzo initialization function
typedef int (__LZO_CDECL *lzo_init_t)(unsigned, int, int, int, int, int, int,
  int, int, int);

// type of pointer to compression level function
typedef int (__LZO_CDECL *lzo_compress_level_t)(const lzo_bytep, lzo_uint,
  lzo_bytep, lzo_uintp, lzo_voidp, const lzo_bytep, lzo_uint, lzo_callback_p,
  int);

#endif

