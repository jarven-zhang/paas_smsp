#ifndef SMGPBASE_H_
#define SMGPBASE_H_
#include "platform.h"
#include <iostream>
#include <string.h>
#include "readerEx.h"
#include "writerEx.h"
#include "CmdCode.h"
#include <stdio.h>

using namespace pack;
using namespace std;


//SMGP错误码，跳过的均为保留
typedef enum
{
    SMGP_SUCCESS							= 0,    //成功
    SMGP_ERROR_MSG_FORMAT					= 3,    //消息格式错
    SMGP_ERROR_ILLEGAL_MSG_LENGTH			= 4,    //非法短消息长度（MsgLength）
    SMGP_ERROR_ILLEGAL_FEECODE				= 5,    //非法资费代码（FeeCode）
    SMGP_ERROR_ILLEGAL_SERVICEID			= 7,    //非法业务代码（ServiceId）
    SMGP_ERROR_SYS_BUSY						= 8,    //系统忙
    SMGP_ERROR_ILLEGAL_SPID					= 10,    //非法SP编号
    SMGP_ERROR_ILLEGAL_MSG_FORMAT			= 11,    //非法信息格式（MsgFormat）
    SMGP_ERROR_ILLEGAL_FEETYPE				= 12,    //非法资费类别（FeeType）
    SMGP_ERROR_ILLEGAL_VALIDTIME			= 13,    //非法存活有效期（ValidTime）
    SMGP_ERROR_ILLEGAL_ATTIME				= 14,    //非法定时发送时间（AtTime）
    SMGP_ERROR_ILLEGAL_CHARGE_TERMID		= 15,    //非法计费号码（ChargeTermId）
    SMGP_ERROR_ILLEGAL_DEST_TERMID			= 16,    //非法目标号码（DestTermId）
    SMGP_ERROR_OPEN_DEST_TERMID_FILE		= 17,    //不能打开目标号码文件（DestTermIdFile）
    SMGP_ERROR_OPEN_MSG_FILE				= 18,    //不能打开短消息内容文件（MsgFile）
    SMGP_ERROR_CONNECT_GW					= 20,    //连接短消息网关失败
    SMGP_ERROR_AUTH							= 21,    //认证错
    SMGP_ERROR_SEND_QUEUE_FULL				= 23,    //发送队列满
    SMGP_ERROR_COMMAND						= 25,    //命令字错
    SMGP_ERROR_SERIAL_NUMBER				= 26,    //序列号错
    SMGP_ERROR_VERSION						= 29,    //版本号不匹配
    SMGP_ERROR_ILLEGAL_MSGTYPE				= 30,    //非法消息类型（MsgType）
    SMGP_ERROR_ILLEGAL_PRIORITY				= 31,    //非法优先级（Priority）
    SMGP_ERROR_ILLEGAL_TIME_FORMAT			= 35,    //非法时间格式
    SMGP_ERROR_ILLEGAL_QUERY_TYPE			= 38,    //非法查询类型（QueryType）
    SMGP_ERROR_ROUTE						= 39,    //路由错误
    SMGP_ERROR_ILLEGAL_FIXED_FEE			= 40,    //非法包月/封顶费（FixedFee）
    SMGP_ERROR_ILLEGAL_SRC_TERMID			= 46,    //非法发送用户号码（SrcTermId）
    SMGP_ERROR_SYS							= 99,    //系统错误
}SMGP_ERROR_CODE;

namespace smsp
{
	class SMGPBase: public Packet
	{
	public:
		SMGPBase();
		virtual ~SMGPBase();
		SMGPBase(const SMGPBase& other)
		{
			this->packetLength_ = other.packetLength_;
			this->requestId_ = other.requestId_;
			this->sequenceId_ = other.sequenceId_;

		}

		SMGPBase& operator =(const SMGPBase& other) 
		{
			this->packetLength_ = other.packetLength_;
			this->requestId_ = other.requestId_;
			this->sequenceId_ = other.sequenceId_;

			return *this;
		}

		const char* getCode(UInt32 code);
	public:
		virtual UInt32 bodySize() const
		{
			return 12;
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
		UInt32 sequenceId_;

	};
}
#endif
