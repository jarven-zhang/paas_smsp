/*****************************************************************************************************
copyright (C),2018-2030, ucpaas .Co.,Ltd.

FileName     : Adapter.h
Author       : Shijh      Version : 1.0    Date: 2018/06/20
Description  : 
Version      : 1.0
Function List:

History      :
<author>       <time>             <version>            <desc>
Shijh         2018/06/20             1.0          build this moudle
******************************************************************************************************/

#ifndef __ADAPTER__H__
#define __ADAPTER__H__

#pragma once

#include <string>
#include <vector>

#define IN                  //函数输入参数标志
#define OUT                 //函数输出参数标志

#ifndef LONG
#define LONG    long
#endif

#ifndef PLONG
#define PLONG   LONG*
#endif


class CAdapter
{
public:
	//**********************************
	//Description: 原子操作-递增
	// Parameter:  [IN] PLONG  Addend 要递增的数
	// Returns:    LONG  32位有符号整数
	//
	//************************************
	static LONG InterlockedIncrement(IN PLONG  Addend);

	//**********************************
	//Description: 原子操作-递减
	// Parameter:  [IN] PLONG  Addend （long*)
	// Returns:    LONG  32位有符号整数
	//************************************
	static LONG InterlockedDecrement(IN PLONG  Addend);

	//**********************************
	//Description: 原子操作-进行Addend 与Value相加
	// Parameter:  IN OUT PLONG  Addend,
	//             IN LONG  Value
	// Returns:    LONG  Addend 相加前的值
	//************************************
	static LONG InterlockedExchangeAdd(IN OUT PLONG  Addend, IN LONG  Value);

	//**********************************
	//Description: 原子操作-交换
	// Parameter:  [IN] [OUT] PLONG  Target （long*)
	//             [IN] LONG  Value
	// Returns:    LONG  32位有符号整数
	//************************************
	static LONG InterlockedExchange(IN OUT PLONG  Target, IN LONG  Value);
};


#endif //!__ADAPTER__H__