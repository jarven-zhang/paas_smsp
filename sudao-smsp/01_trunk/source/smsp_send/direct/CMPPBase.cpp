#include "CMPPBase.h"
#include <limits.h>
#include "SnManager.h"

namespace smsp
{
    UInt32 CMPPBase::g_seqId = 0;
    CMPPBase::CMPPBase()
    {
        _totalLength = 0;
        _commandId = 0;
        _sequenceId = 0;
    }

    CMPPBase::~CMPPBase()
    {

    }

    bool CMPPBase::Pack(Writer& writer) const
    {
        writer(bodySize())(_commandId)(_sequenceId);
        return writer;
    }

    bool CMPPBase::Unpack(Reader& reader)
    {
        if(reader.GetSize() == 0)
        {
            reader(_totalLength);
            return reader;
        }
        reader(_totalLength)(_commandId)(_sequenceId);
        return reader;

    }
    const char* CMPPBase::getCode(UInt32 code) const
    {
        switch (code)
        {
            case CMPP_CONNECT:
                return "CMPP_CONNECT";
                break;
            case CMPP_CONNECT_RESP:
                return "CMPP_CONNECT_RESP";
                break;
            case CMPP_TERMINATE:
                return "CMPP_TERMINATE";
                break;
            case CMPP_TERMINATE_RESP:
                return "CMPP_TERMINATE_RESP";
                break;
            case CMPP_SUBMIT:
                return "CMPP_SUBMIT";
                break;
            case CMPP_SUBMIT_RESP:
                return "CMPP_SUBMIT_RESP";
                break;
            case CMPP_QUERY:
                return "CMPP_QUERY_RESP";
                break;
            case CMPP_ACTIVE_TEST:
                return "CMPP_ACTIVE_TEST";
                break;
            case CMPP_ACTIVE_TEST_RESP:
                return "CMPP_ACTIVE_TEST_RESP";
                break;
            case CMPP_DELIVER_RESP:
                return "CMPP_DELIVER_RESP";
                break;
            case CMPP_DELIVER:
                return "CMPP_DELIVER";
                break;
            case CMPP_MT_ROUTE_UPDATE_RESP:
                return "CMPP_MT_ROUTE_UPDATE_RESP";
                break;
            case CMPP_MO_ROUTE_UPDATE_RESP:
                return "CMPP_MO_ROUTE_UPDATE_RESP";
                break;
            case CMPP_PUSH_MT_ROUTE_UPDATE_RESP:
                return "CMPP_PUSH_MT_ROUTE_UPDATE_RESP";
                break;
            case CMPP_PUSH_MO_ROUTE_UPDATE_RESP:
                return "CMPP_PUSH_MO_ROUTE_UPDATE_RESP";
                break;
            case CMPP_MO_ROUTE_RESP:
                return "CMPP_MO_ROUTE_RESP";
                break;
            case CMPP_GET_ROUTE_RESP:
                return "CMPP_GET_ROUTE_RESP";
                break;
            case CMPP_MT_ROUTE_RESP:
                return "CMPP_MT_ROUTE_RESP";
                break;
            case CMPP_FWD_RESP:
                return "CMPP_FWD_RESP";
                break;
            case CMPP_CANCEL_RESP:
                return "CMPP_CANCEL_RESP";
                break;
            case CMPP_CANCEL:
                return "CMPP_CANCEL";
                break;
            default:
                return "CMPP_OTHER";
                break;
        }
        return "";

    }

    UInt32 CMPPBase::getSeqID()
    {
        if (g_seqId >= UINT_MAX)
        {
            g_seqId = 0;
        }
        g_seqId++;
        return g_seqId;
    }

} /* namespace smsp */
