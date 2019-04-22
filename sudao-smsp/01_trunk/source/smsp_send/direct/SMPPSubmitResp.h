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
        virtual bool Unpack(pack::Reader &reader);
    public:
        std::string _msgID;
    };

} /* namespace smsp */
#endif /* SMPPSUBMITRESP_H_ */
