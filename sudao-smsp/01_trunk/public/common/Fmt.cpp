#include "Fmt.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "boost/static_assert.hpp"
#include "boost/type_traits/is_arithmetic.hpp"

///////////////////////////////////////////////////////////////

template<typename T>
T to_uint(cs_t val)
{
    return atoi(val.data());
}

UInt16 to_uint16(cs_t val) {return atoi(val.data());}
UInt32 to_uint32(cs_t val) {return atoi(val.data());}
UInt64 to_uint64(cs_t val) {return strtoul(val.data(), NULL, 0);}
double to_double(cs_t val) {return atof(val.data());}


// Explicit instantiations

template UInt16 to_uint<UInt16>(cs_t str);
template UInt32 to_uint<UInt32>(cs_t str);
template UInt64 to_uint<UInt64>(cs_t str);

///////////////////////////////////////////////////////////////

template<typename T>
string Fmt(const char* fmt, T val)
{
  BOOST_STATIC_ASSERT(boost::is_arithmetic<T>::value == true);

  char buf[32] = {0};
  int length = snprintf(buf, sizeof buf, fmt, val);
  assert(static_cast<size_t>(length) < sizeof buf);
  return string(buf);
}

template string Fmt(const char* fmt, char);
template string Fmt(const char* fmt, short);
template string Fmt(const char* fmt, unsigned short);
template string Fmt(const char* fmt, int);
template string Fmt(const char* fmt, unsigned int);
template string Fmt(const char* fmt, long);
template string Fmt(const char* fmt, unsigned long);
template string Fmt(const char* fmt, long long);
template string Fmt(const char* fmt, unsigned long long);
template string Fmt(const char* fmt, float);
template string Fmt(const char* fmt, double);

///////////////////////////////////////////////////////////////

template<int SIZE>
string Fmt(const char* fmt, cs_t val)
{
    char buf[SIZE] = {0};
    int length = snprintf(buf, sizeof buf, fmt, val.data());
    assert(static_cast<size_t>(length) < sizeof buf);
    return string(buf);
}

// Explicit instantiations

template string Fmt<32>(const char* fmt, cs_t);
template string Fmt<64>(const char* fmt, cs_t);
template string Fmt<128>(const char* fmt, cs_t);

///////////////////////////////////////////////////////////////

template<int SIZE>
string Fmt(const char* fmt, ...)
{
    char buf[SIZE] = {0};
    va_list argp;
    va_start(argp, fmt);
    int length = vsnprintf(buf, sizeof buf, fmt, argp);
    va_end(argp);
    assert(static_cast<size_t>(length) < sizeof buf);
    return string(buf);
}

// Explicit instantiations

template string Fmt<16>(const char* fmt, ...);
template string Fmt<32>(const char* fmt, ...);
template string Fmt<64>(const char* fmt, ...);
template string Fmt<128>(const char* fmt, ...);
template string Fmt<256>(const char* fmt, ...);
template string Fmt<1024>(const char* fmt, ...);

///////////////////////////////////////////////////////////////

string to_string(int value) {return Fmt("%d", value);}
string to_string(long value) {return Fmt("%ld", value);}
string to_string(long long value) {return Fmt("%lld", value);}
string to_string(unsigned int value) {return Fmt("%u", value);}
string to_string(unsigned long value) {return Fmt("%lu", value);}
string to_string(unsigned long long value) {return Fmt("%llu", value);}
string to_string(float value) {return Fmt("%f", value);}
string to_string(double value) {return Fmt("%f", value);}
string to_string(long double value) {return Fmt("%lf", value);}


