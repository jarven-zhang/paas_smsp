#include "SMPPSubmitResp.h"
#include "main.h"
namespace smsp
{

    SMPPSubmitResp::SMPPSubmitResp()
    {
        _commandId = SUBMIT_SM_RESP;
        _uMsgID = 0;
        m_strMsgId = "";
    }

    SMPPSubmitResp::~SMPPSubmitResp()
    {
        // TODO Auto-generated destructor stub
    }

    UInt32 SMPPSubmitResp::bodySize() const
    {
        return (m_strMsgId.length()+1) + SMPPBase::bodySize();
    }

    bool SMPPSubmitResp::Pack(Writer& writer) const
    {
        SMPPBase::Pack(writer);
		writer((UInt32)(m_strMsgId.length() + 1), m_strMsgId);
        return writer;
    }

} /* namespace smsp */
