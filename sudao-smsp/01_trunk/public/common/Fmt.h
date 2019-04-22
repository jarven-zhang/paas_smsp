#ifndef THREAD_RULELOADTHREAD_FMT_H
#define THREAD_RULELOADTHREAD_FMT_H

#include "platform.h"


template<typename T>
T to_uint(cs_t val);

UInt16 to_uint16(cs_t val);
UInt32 to_uint32(cs_t val);
UInt64 to_uint64(cs_t val);
double to_double(cs_t val);

template<typename T>
string Fmt(const char* fmt, T val);

template<int SIZE>
string Fmt(const char* fmt, cs_t val);

template<int SIZE>
string Fmt(const char* fmt, ...);

string to_string(int value);
string to_string(long value);
string to_string(long long value);
string to_string(unsigned value);
string to_string(unsigned long value);
string to_string(unsigned long long value);
string to_string(float value);
string to_string(double value);
string to_string(long double value);

#endif // THREAD_RULELOADTHREAD_FMT_H
