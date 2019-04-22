#ifndef SMPPSUBMITRESP_H_
#define SMPPSUBMITRESP_H_
#include "SMPPBase.h"
namespace smsp
{

    class SMPPSubmitResp:public SMPPBase
    {
    public:
        SMPPSubmitResp();
        virtual ~SMPPSubmitResp();

        SMPPSubmitResp(const SMPPBase& other)
        {
            this->_commandLength=other._commandLength;
            this->_commandStatus=other._commandStatus;
            this->_sequenceNum=other._sequenceNum;
            this->_commandId=other._commandId;
        }
    public:
		virtual UInt32 bodySize() const;
        bool Pack(pack::Writer &writer) const;

		
        UInt64 _uMsgID;  ////max 9
        std::string m_strMsgId;
    };

} /* namespace smsp */
#endif /* SMPPSUBMITRESP_H_ */
