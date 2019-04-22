
#include "initHelper.h"

initHelper * initHelper::m_pInstance = new initHelper();

initHelper::initHelper()
{
	m_bInitOver = false;
	pthread_rwlock_init(&m_rwLock, NULL);
}

initHelper * initHelper::GetInstance()
{
	return m_pInstance;
}

void initHelper::setInitState(bool bInitState)
{
	pthread_rwlock_rdlock(&m_rwLock);
	m_bInitOver = bInitState;
	pthread_rwlock_unlock(&m_rwLock);
}

bool initHelper::getInitState()
{
	bool ret;

	pthread_rwlock_rdlock(&m_rwLock);
	ret = m_bInitOver;
	pthread_rwlock_unlock(&m_rwLock);

	return ret;
}

bool initHelper::checkInitState()
{
	return getInitState();
}


