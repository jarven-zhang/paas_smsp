#ifndef SMPPDELIVERRESP_H_
#define SMPPDELIVERRESP_H_
#include "SMPPBase.h"
namespace smsp
{
    class SMPPDeliverResp: public SMPPBase
    {
    public:
        SMPPDeliverResp();
        virtual ~SMPPDeliverResp();

        SMPPDeliverResp(const SMPPBase& other)
        {
            this->_commandLength=other._commandLength;
            this->_commandStatus=other._commandStatus;
            this->_sequenceNum=other._sequenceNum;
            this->_commandId=DELIVER_SM_RESP;
        }

    public:
        virtual bool Unpack(pack::Reader &reader);
    public:
        std::string _msgId;
    };

} /* namespace smsp */

#endif /* SMPPDELIVERRESP_H_ */
