#ifndef SMPPDELIVERREQ_H_
#define SMPPDELIVERREQ_H_
#include "SMPPBase.h"
namespace smsp
{

    class SMPPDeliverReq : public SMPPBase
    {
    public:
        SMPPDeliverReq();
        virtual ~SMPPDeliverReq();

        SMPPDeliverReq(const SMPPBase& other)
        {
            this->_commandLength=other._commandLength;
            this->_commandStatus=other._commandStatus;
            this->_sequenceNum=other._sequenceNum;
            this->_commandId=DELIVER_SM_RESP;
        }

    public:
        virtual UInt32 bodySize() const;
        virtual bool Pack(pack::Writer &writer) const;
    public:
        std::string _msgId;
    };

} /* namespace smsp */
#endif /* SMPPDELIVERREQ_H_ */
