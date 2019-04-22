#include "SMPPBase.h"
#include <limits.h>
#include <arpa/inet.h>
#include "SnManager.h"
#include "main.h"

UInt32 SMPPBase::g_seqId = 0;
SMPPBase::SMPPBase()
{
    _commandLength= 0;
    _commandId = 0;
    _commandStatus = 0;
    _sequenceNum = 0;
}

SMPPBase::~SMPPBase()
{

}

bool SMPPBase::Pack(pack::Writer& writer) const
{
    writer(bodySize())(_commandId)(_commandStatus)(_sequenceNum);
    //writer(htonl(bodySize()))(htonl(_commandId))(htonl(_commandStatus))(htonl(_sequenceNum));
    ////LogDebug("_commandLength[%d],_commandId[0x%x],_sequenceNum[%d]",bodySize(),_commandId,_sequenceNum);
    return writer;
}

bool SMPPBase::Unpack(pack::Reader& reader)
{
    if(reader.GetSize() == 0)
    {
        reader(_commandLength);
        return reader;
    }
    reader(_commandLength)(_commandId)(_commandStatus)(_sequenceNum);
    ////LogDebug("_commandLength[%d],_commandId[0x%x],_commandStatus[%d],_sequenceNum[%d]",_commandLength,_commandId,_commandStatus,_sequenceNum);
    return reader;

}
const char* SMPPBase::getCode(UInt32 code) const
{
    switch (code)
    {
        case BIND_RECEIVER:
            return "BIND_RECEIVER";
            break;
        case BIND_RECEIVER_RESP:
            return "BIND_RECEIVER_RESP";
            break;
        case BIND_TRANSMITTER:
            return "BIND_TRANSMITTER";
            break;
        case BIND_TRANSMITTER_RESP:
            return "BIND_TRANSMITTER_RESP";
            break;
        case QUERY_SM:
            return "QUERY_SM";
            break;
        case QUERY_SM_RESP:
            return "QUERY_SM_RESP";
            break;
        case SUBMIT_SM:
            return "SUBMIT_SM";
            break;
        case SUBMIT_SM_RESP:
            return "SUBMIT_SM_RESP";
            break;
        case DELIVER_SM:
            return "DELIVER_SM";
            break;
        case DELIVER_SM_RESP:
            return "DELIVER_SM_RESP";
            break;
        case UNBIND:
            return "UNBIND";
            break;
        case UNBIND_RESP:
            return "UNBIND_RESP";
            break;
        case REPLACE_SM:
            return "REPLACE_SM";
            break;
        case REPLACE_SM_RESP:
            return "REPLACE_SM_RESP";
            break;
        case CANCEL_SM:
            return "CANCEL_SM";
            break;
        case CANCEL_SM_RESP:
            return "CANCEL_SM_RESP";
            break;
        case BIND_TRANSCEIVER:
            return "BIND_TRANSCEIVER";
            break;
        case BIND_TRANSCEIVER_RESP:
            return "BIND_TRANSCEIVER_RESP";
            break;
        case OUTBIND:
            return "OUTBIND";
            break;
        case ENQUIRE_LINK:
            return "ENQUIRE_LINK";
            break;
        case ENQUIRE_LINK_RESP:
            return "ENQUIRE_LINK_RESP";
            break;
        default:
            return "SMPP_OTHER";
            break;
    }
    return "";

}

UInt32 SMPPBase::getSeqID()
{
    if (g_seqId >= UINT_MAX)
    {
        g_seqId = 0;
    }
    g_seqId++;
    return g_seqId;
}


