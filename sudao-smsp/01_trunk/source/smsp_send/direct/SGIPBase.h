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
