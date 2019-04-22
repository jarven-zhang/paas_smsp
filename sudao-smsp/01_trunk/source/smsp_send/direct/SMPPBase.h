#ifndef SMPPBASE_H_
#define SMPPBASE_H_
#include "platform.h"
#include <iostream>
#include <string.h>

#include "readerEx.h"
#include "writerEx.h"
#include "CmdCode.h"

using namespace std;

class SMPPBase: public pack::Packet
{
public:
    SMPPBase();
    virtual ~SMPPBase();
    SMPPBase(const SMPPBase& other)
    {
        _commandLength = other._commandLength;
        _commandId = other._commandId;
        _commandStatus = other._commandStatus;
        _sequenceNum = other._sequenceNum;
    }

    SMPPBase& operator =(const SMPPBase& other)
    {
        _commandLength = other._commandLength;
        _commandId = other._commandId;
        _commandStatus = other._commandStatus;
        _sequenceNum = other._sequenceNum;

        return *this;
    }
    const char* getCode(UInt32 code)const;
public:

    virtual UInt32 bodySize() const
    {
        return 16;
    }
    virtual bool Pack(pack::Writer &writer) const;
    virtual bool Unpack(pack::Reader &reader);

    virtual std::string Dump() const
    {
        return "";
    }
    static UInt32 getSeqID();
    static UInt32 g_seqId;
public:
    //header
    UInt32 _commandLength;
    UInt32 _commandId;
    UInt32 _commandStatus;
    UInt32 _sequenceNum;
private:

};

#endif /* SMPPBASE_H_ */
