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


//SMGP�����룬�����ľ�Ϊ����
typedef enum
{
    SMGP_SUCCESS							= 0,    //�ɹ�
    SMGP_ERROR_MSG_FORMAT					= 3,    //��Ϣ��ʽ��
    SMGP_ERROR_ILLEGAL_MSG_LENGTH			= 4,    //�Ƿ�����Ϣ���ȣ�MsgLength��
    SMGP_ERROR_ILLEGAL_FEECODE				= 5,    //�Ƿ��ʷѴ��루FeeCode��
    SMGP_ERROR_ILLEGAL_SERVICEID			= 7,    //�Ƿ�ҵ����루ServiceId��
    SMGP_ERROR_SYS_BUSY						= 8,    //ϵͳæ
    SMGP_ERROR_ILLEGAL_SPID					= 10,    //�Ƿ�SP���
    SMGP_ERROR_ILLEGAL_MSG_FORMAT			= 11,    //�Ƿ���Ϣ��ʽ��MsgFormat��
    SMGP_ERROR_ILLEGAL_FEETYPE				= 12,    //�Ƿ��ʷ����FeeType��
    SMGP_ERROR_ILLEGAL_VALIDTIME			= 13,    //�Ƿ������Ч�ڣ�ValidTime��
    SMGP_ERROR_ILLEGAL_ATTIME				= 14,    //�Ƿ���ʱ����ʱ�䣨AtTime��
    SMGP_ERROR_ILLEGAL_CHARGE_TERMID		= 15,    //�Ƿ��ƷѺ��루ChargeTermId��
    SMGP_ERROR_ILLEGAL_DEST_TERMID			= 16,    //�Ƿ�Ŀ����루DestTermId��
    SMGP_ERROR_OPEN_DEST_TERMID_FILE		= 17,    //���ܴ�Ŀ������ļ���DestTermIdFile��
    SMGP_ERROR_OPEN_MSG_FILE				= 18,    //���ܴ򿪶���Ϣ�����ļ���MsgFile��
    SMGP_ERROR_CONNECT_GW					= 20,    //���Ӷ���Ϣ����ʧ��
    SMGP_ERROR_AUTH							= 21,    //��֤��
    SMGP_ERROR_SEND_QUEUE_FULL				= 23,    //���Ͷ�����
    SMGP_ERROR_COMMAND						= 25,    //�����ִ�
    SMGP_ERROR_SERIAL_NUMBER				= 26,    //���кŴ�
    SMGP_ERROR_VERSION						= 29,    //�汾�Ų�ƥ��
    SMGP_ERROR_ILLEGAL_MSGTYPE				= 30,    //�Ƿ���Ϣ���ͣ�MsgType��
    SMGP_ERROR_ILLEGAL_PRIORITY				= 31,    //�Ƿ����ȼ���Priority��
    SMGP_ERROR_ILLEGAL_TIME_FORMAT			= 35,    //�Ƿ�ʱ���ʽ
    SMGP_ERROR_ILLEGAL_QUERY_TYPE			= 38,    //�Ƿ���ѯ���ͣ�QueryType��
    SMGP_ERROR_ROUTE						= 39,    //·�ɴ���
    SMGP_ERROR_ILLEGAL_FIXED_FEE			= 40,    //�Ƿ�����/�ⶥ�ѣ�FixedFee��
    SMGP_ERROR_ILLEGAL_SRC_TERMID			= 46,    //�Ƿ������û����루SrcTermId��
    SMGP_ERROR_SYS							= 99,    //ϵͳ����
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
