#ifndef CMPP3BASE_H_
#define CMPP3BASE_H_
#include "platform.h"
#include <iostream>
#include <string.h>
#include <vector>
#include "readerEx.h"
#include "writerEx.h"
#include "CmdCode.h"
#include <stdio.h>

using namespace pack;
using namespace std;

namespace cmpp3
{
	char * timestamp(char *buf);
	char * timeYear(char *buf);
	char * timestampYear(char *buf);
    class CMPPBase: public Packet
    {
    public:
        CMPPBase();
        virtual ~CMPPBase();
        CMPPBase(const CMPPBase& other)
        {
            _totalLength = other._totalLength;
            _commandId = other._commandId;
            _sequenceId = other._sequenceId;
        }

        CMPPBase& operator =(const CMPPBase& other)
        {
            _totalLength = other._totalLength;
            _commandId = other._commandId;
            _sequenceId = other._sequenceId;
            return *this;
        }
        const char* getCode(UInt32 code)const;
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
        UInt32 _totalLength;
        UInt32 _commandId;
        UInt32 _sequenceId;
    private:

    };

} /* namespace smsp */
#endif /* CMPPBASE_H_ */
