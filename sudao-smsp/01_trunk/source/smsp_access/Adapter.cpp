/*****************************************************************************************************
copyright (C),2018-2030, ucpaas .Co.,Ltd.

FileName     : Adapter.cpp
Author       : Shijh      Version : 1.0    Date: 2018/06/20
Description  :
Version      : 1.0
Function List:

History      :
<author>       <time>             <version>            <desc>
Shijh       2018/06/20          1.0          build this moudle
******************************************************************************************************/

#include "Adapter.h"

//**********************************
// Description: 原子操作-递增
// Parameter:  [IN] PLONG  Addend 要递增的数
// Returns:    LONG  32位有符号整数
//
//************************************
LONG CAdapter::InterlockedIncrement(IN PLONG  Addend)
{
    __gnu_cxx::__atomic_add((_Atomic_word *)Addend, 1);
    return *Addend;
}

//**********************************
// Description: 原子操作-递减
// Parameter:  [IN] PLONG  Addend （long*)
// Returns:    LONG  32位有符号整数
//************************************
LONG CAdapter::InterlockedDecrement(IN PLONG  Addend)
{
    __gnu_cxx::__atomic_add((_Atomic_word *)Addend, -1);
    return *Addend;
}

//**********************************
// Description: 原子操作-进行Addend 与Value相加
// Parameter:  IN OUT PLONG  Addend,
//             IN LONG  Value
// Returns:    LONG  Addend 相加前的值
//************************************
LONG CAdapter::InterlockedExchangeAdd(IN OUT PLONG  Addend, IN LONG  Value)
{
    return __gnu_cxx::__exchange_and_add((_Atomic_word *)Addend, Value);
}

//**********************************
// Description: 原子操作-交换
// Parameter:  [IN] [OUT] PLONG  Target （long*)
//             [IN] LONG  Value
// Returns:    LONG  32位有符号整数
//************************************
LONG CAdapter::InterlockedExchange(IN OUT PLONG  Target, IN LONG  Value)
{
    return __gnu_cxx::__exchange_and_add((_Atomic_word *)Target, Value - *Target);
}
