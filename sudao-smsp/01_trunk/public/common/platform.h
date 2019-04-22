#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <string>
#include <stdint.h>
#include "BusTypes.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

using std::string;
using namespace BusType;

typedef const string& cs_t;


typedef int8_t Int8;
typedef uint8_t UInt8;

typedef int16_t Int16;
typedef uint16_t UInt16;

typedef int32_t Int32;
typedef uint32_t UInt32;

typedef int64_t Int64;
typedef uint64_t UInt64;

// by add sjh
#ifndef TINYINT
#define TINYINT    unsigned char
#endif

const Int8          INVALID_INT8    = -1;
const UInt8         INVALID_UINT8   = 0xFF;
const Int16         INVALID_INT16   = -1;
const UInt16        INVALID_UINT16  = 0xFFFF;
const Int32         INVALID_INT32   = -1;
const UInt32        INVALID_UINT32  = 0xFFFFFFFF;
const Int64         INVALID_INT64   = -1;
const UInt64        INVALID_UINT64  = 0xFFFFFFFFFFFFFFFF;
const std::string   INVALID_STR     = "";


#if defined(__APPLE__) && defined(__GNUC__)
#define OS_MACX
#elif defined(__MACOSX__)
#define OS_MACX
#elif !defined(SAG_COM) && (defined(WIN64) || defined(_WIN64) || defined(__WIN64__))
#define OS_WIN32
#define OS_WIN64
#elif !defined(SAG_COM) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#define OS_WIN32
#elif defined(__MWERKS__) && defined(__INTEL__)
#define OS_WIN32
//#elif defined(__linux__) || defined(__linux)
//#define OS_LINUX
#endif

#endif /* PLATFORM_H_ */

