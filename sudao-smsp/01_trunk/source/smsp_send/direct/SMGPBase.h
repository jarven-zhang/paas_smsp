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
#endif
