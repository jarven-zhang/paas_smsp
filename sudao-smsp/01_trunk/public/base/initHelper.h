#ifndef __INIT_HELPER_H__
#define __INIT_HELPER_H__
#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>

#ifndef SET_ALL_THREAD_INIT_OK
#define SET_ALL_THREAD_INIT_OK()  do{initHelper::GetInstance()->setInitState(true);}while(0);
#endif

#ifndef LOOP_UNTIL_ALL_THREAD_INIT_OK
#define LOOP_UNTIL_ALL_THREAD_INIT_OK()\
	while(1)\
	{\
		if(initHelper::GetInstance()->checkInitState())\
		{\
			break;\
		}\
		else\
		{\
			usleep(1000);\
		}\
	}\
	printf("\r\nFile:%s, Line:%d, Function:%s, --- All thread init over!\r\n", __FILE__, __LINE__ , __FUNCTION__);
#endif

class initHelper
{

private:

	initHelper();

public:

	static initHelper * GetInstance();
	bool   getInitState();
	void   setInitState(bool bInitState);
	bool   checkInitState();

private:
	pthread_rwlock_t 	m_rwLock;
	bool   				m_bInitOver;
	static initHelper * m_pInstance;
};

#endif