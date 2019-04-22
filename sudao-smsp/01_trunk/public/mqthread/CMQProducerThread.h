/******************************************************************************

  Copyright (C), 2001-2011, DCN Co., Ltd.

 ******************************************************************************
  File Name     : CMQProducerThread.h
  Version       : Initial Draft
  Author        : fxh
  Created       : 2016/10/13
  Last Modified :
  Description   : rabbitMQ Producer Thread
  Function List :
  History       :
  1.Date        : 2016/10/13
    Author      : fxh
    Modification: Created file

******************************************************************************/
#ifndef __CMQPRODUCERTHREAD__
#define __CMQPRODUCERTHREAD__

#include <string.h>
#include "Thread.h"

extern "C"
{
	#include "amqp_tcp_socket.h"
	#include "amqp.h"
	#include "amqp_framing.h"
}

class TMQPublishReqMsg:public TMsg
{
public:
	TMQPublishReqMsg()
	{
		m_strExchange = "";
		m_strRoutingKey = "";
		m_strData = "";
		m_strKey = "";
        m_iPriority = 0;
	}
	string m_strExchange;
	string m_strRoutingKey;
	string m_strData;
	string m_strKey;
    int m_iPriority;
};


class CMQProducerThread:public CThread
{

public:
    CMQProducerThread(const char *name);
    ~CMQProducerThread();
    virtual bool Init(string strIp, unsigned int uPort,string strUserName,string strPassWord);

	virtual void MainLoop(void);
    virtual void HandleMsg(TMsg* pMsg);

	bool RabbitMQConnect();
	bool close();

	bool m_linkState;

	void HandlePublish(TMsg* pMsg);

protected:
	string m_strIp;
	unsigned int m_uPort;
	string m_strUserName;
	string m_strPassWord;
	TMQPublishReqMsg* m_errMsg;	//add by fangjinxiong20161031, after mq link break, this thread will lose on msg. m_errMsg to save this msg

	amqp_connection_state_t m_MQconn;
	amqp_basic_properties_t m_Props;

};

#endif /////__CMQPRODUCERTHREAD__

