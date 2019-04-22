/******************************************************************************

  Copyright (C), 2001-2011, DCN Co., Ltd.

 ******************************************************************************
  File Name     : CDBMQProducerThread.h
  Version       : Initial Draft
  Author        : ljl
  Created       : 2017/10/19
  Last Modified :
  Description   : DB rabbitMQ Producer Thread
  Function List :
  History       :
  

******************************************************************************/
#ifndef __CDBMQPRODUCERTHREAD__
#define __CDBMQPRODUCERTHREAD__


#include <vector>
#include "CMQProducerThread.h"
#include "ComponentRefMq.h"
#include "MqConfig.h"
#define DBMQ_NUM  5

class CDBMQProducerThread:public CMQProducerThread
{

public:
    CDBMQProducerThread(const char *name);
    ~CDBMQProducerThread();
	bool Init(string strIp, unsigned int uPort,string strUserName,string strPassWord);
    virtual void HandleMsg(TMsg* pMsg);
	void GetMQInfo(TMsg* pMsg);
	bool MQinfoInit();
	
private:
	string GetReportUid(string strData);
	vector<MqConfig> m_vDBMQInfo;
};

#endif /////__CMQPRODUCERTHREAD__

