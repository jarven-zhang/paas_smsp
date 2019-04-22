#ifndef SGIPBASE_H_
#define SGIPBASE_H_
#include "platform.h"
#include <iostream>
#include <string.h>

#include "readerEx.h"
#include "writerEx.h"
#include "CmdCode.h"
#include <stdio.h>

using namespace pack;
using namespace std;


//SGIP错误码，跳过的均为保留
typedef enum
{
    SGIP_SUCCESS							= 0,    //无错误，命令正确接收
    SGIP_ERROR_ILLEGAL_LOGIN				= 1,    //非法登录，如登录名、口令出错、登录名与口令不符等。
    SGIP_ERROR_REPEAT_LOGIN					= 2,    //重复登录，如在同一TCP/IP连接中连续两次以上请求登录。
    SGIP_ERROR_MUCH_CONNECT					= 3,    //连接过多，指单个节点要求同时建立的连接数过多。
    SGIP_ERROR_LOGINTYPE					= 4,    //登录类型错，指bind命令中的logintype字段出错。
    SGIP_ERROR_PARAM_FORMAT					= 5,    //参数格式错，指命令中参数值与参数类型不符或与协议规定的范围不符。
    SGIP_ERROR_ILLEGAL_TERMID				= 6,    //非法手机号码，协议中所有手机号码字段出现非86130号码或手机号码前未加“86”时都应报错。
    SGIP_ERROR_MSGID						= 7,    //消息ID错
    SGIP_ERROR_MSG_LENGTH					= 8,    //信息长度错
    SGIP_ERROR_ILLEGAL_SERIAL_NUMBER		= 9,    //非法序列号，包括序列号重复、序列号格式错误等
    SGIP_ERROR_ILLEGAL_OPERATE_GNS			= 10,    //非法操作GNS
    SGIP_ERROR_NODE_BUSY					= 11,    //节点忙，指本节点存储队列满或其他原因，暂时不能提供服务的情况
    SGIP_ERROR_UNRECHABLE_DEST				= 21,    //目的地址不可达，指路由表存在路由且消息路由正确但被路由的节点暂时不能提供服务的情况
    SGIP_ERROR_ROUTE						= 22,    //路由错，指路由表存在路由但消息路由出错的情况，如转错SMG等
    SGIP_ERROR_NOEXIST_ROUTE				= 23,    //路由不存在，指消息路由的节点在路由表中不存在
    SGIP_ERROR_CHARGE_TERMID				= 24,    //计费号码无效，鉴权不成功时反馈的错误信息
    SGIP_ERROR_COMMUNUCATION_TERM			= 25,    //用户不能通信（如不在服务区、未开机等情况）
    SGIP_ERROR_TERM_NOFREE_MEM				= 26,    //手机内存不足
    SGIP_ERROR_TERM_UNSUPPORT				= 27,    //手机不支持短消息
    SGIP_ERROR_TERM_RECV_MSG				= 28,    //手机接收短消息出现错误
    SGIP_ERROR_UNKNOW_TERM					= 29,    //不知道的用户
    SGIP_ERROR_NOTAVAILABLE_FEATURE			= 30,    //不提供此功能
    SGIP_ERROR_ILLEGAL_TERM					= 31,    //非法设备
    SGIP_ERROR_SYS							= 32,    //系统失败
    SGIP_ERROR_QUEUE_FULL					= 33,    //短信中心队列满
}SGIP_ERROR_CODE;

class SGIPBase: public Packet 
{
public:
	SGIPBase();

	virtual ~SGIPBase();
	SGIPBase(const SGIPBase& other)
	{
		this->packetLength_=other.packetLength_;
		this->requestId_=other.requestId_;
		this->sequenceIdNode_=other.sequenceIdNode_;
		this->sequenceIdTime_=other.sequenceIdTime_;
		this->sequenceId_=other.sequenceId_;
	}

	SGIPBase& operator =(const SGIPBase& other)
	{
		this->packetLength_=other.packetLength_;
		this->requestId_=other.requestId_;
		this->sequenceIdNode_=other.sequenceIdNode_;
		this->sequenceIdTime_=other.sequenceIdTime_;
		this->sequenceId_=other.sequenceId_;

		return *this;
	}

	void setSeqIdNode(UInt32 sequenceIdNode)
	{
		sequenceIdNode_=sequenceIdNode;
	}

	const char* getCode(UInt32 code);
public:
	virtual UInt32 bodySize() const
	{
		return 20;
	}
	virtual bool Pack(Writer &writer) const;
	virtual bool Unpack(Reader &reader);

	virtual std::string Dump() const
	{
		return "";
	}
	
	static UInt32 getSeqID();
	static UInt32 g_seqId;

public:
	//header
	UInt32 packetLength_;
	UInt32 requestId_;
	UInt32 sequenceIdNode_;
	UInt32 sequenceIdTime_;
	UInt32 sequenceId_;

	static UInt32 g_nNode;

};
#endif
