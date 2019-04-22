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


//SGIP�����룬�����ľ�Ϊ����
typedef enum
{
    SGIP_SUCCESS							= 0,    //�޴���������ȷ����
    SGIP_ERROR_ILLEGAL_LOGIN				= 1,    //�Ƿ���¼�����¼�������������¼���������ȡ�
    SGIP_ERROR_REPEAT_LOGIN					= 2,    //�ظ���¼������ͬһTCP/IP�����������������������¼��
    SGIP_ERROR_MUCH_CONNECT					= 3,    //���ӹ��ָ࣬�����ڵ�Ҫ��ͬʱ���������������ࡣ
    SGIP_ERROR_LOGINTYPE					= 4,    //��¼���ʹ�ָbind�����е�logintype�ֶγ���
    SGIP_ERROR_PARAM_FORMAT					= 5,    //������ʽ��ָ�����в���ֵ��������Ͳ�������Э��涨�ķ�Χ������
    SGIP_ERROR_ILLEGAL_TERMID				= 6,    //�Ƿ��ֻ����룬Э���������ֻ������ֶγ��ַ�86130������ֻ�����ǰδ�ӡ�86��ʱ��Ӧ����
    SGIP_ERROR_MSGID						= 7,    //��ϢID��
    SGIP_ERROR_MSG_LENGTH					= 8,    //��Ϣ���ȴ�
    SGIP_ERROR_ILLEGAL_SERIAL_NUMBER		= 9,    //�Ƿ����кţ��������к��ظ������кŸ�ʽ�����
    SGIP_ERROR_ILLEGAL_OPERATE_GNS			= 10,    //�Ƿ�����GNS
    SGIP_ERROR_NODE_BUSY					= 11,    //�ڵ�æ��ָ���ڵ�洢������������ԭ����ʱ�����ṩ��������
    SGIP_ERROR_UNRECHABLE_DEST				= 21,    //Ŀ�ĵ�ַ���ɴָ·�ɱ����·������Ϣ·����ȷ����·�ɵĽڵ���ʱ�����ṩ��������
    SGIP_ERROR_ROUTE						= 22,    //·�ɴ�ָ·�ɱ����·�ɵ���Ϣ·�ɳ�����������ת��SMG��
    SGIP_ERROR_NOEXIST_ROUTE				= 23,    //·�ɲ����ڣ�ָ��Ϣ·�ɵĽڵ���·�ɱ��в�����
    SGIP_ERROR_CHARGE_TERMID				= 24,    //�ƷѺ�����Ч����Ȩ���ɹ�ʱ�����Ĵ�����Ϣ
    SGIP_ERROR_COMMUNUCATION_TERM			= 25,    //�û�����ͨ�ţ��粻�ڷ�������δ�����������
    SGIP_ERROR_TERM_NOFREE_MEM				= 26,    //�ֻ��ڴ治��
    SGIP_ERROR_TERM_UNSUPPORT				= 27,    //�ֻ���֧�ֶ���Ϣ
    SGIP_ERROR_TERM_RECV_MSG				= 28,    //�ֻ����ն���Ϣ���ִ���
    SGIP_ERROR_UNKNOW_TERM					= 29,    //��֪�����û�
    SGIP_ERROR_NOTAVAILABLE_FEATURE			= 30,    //���ṩ�˹���
    SGIP_ERROR_ILLEGAL_TERM					= 31,    //�Ƿ��豸
    SGIP_ERROR_SYS							= 32,    //ϵͳʧ��
    SGIP_ERROR_QUEUE_FULL					= 33,    //�������Ķ�����
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
