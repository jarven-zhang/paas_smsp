#ifndef SMPPCONNECTRESP_H_
#define SMPPCONNECTRESP_H_
#include "SMPPBase.h"
namespace smsp
{

    class SMPPConnectResp: public SMPPBase
    {
    public:
        enum CMPPCONNSTATE
        {
            CONNSTATESUC = 0, CONNSTSTEFAILED,
        };
        SMPPConnectResp();
        SMPPConnectResp(const SMPPBase& other)
        {
            this->_commandLength=other._commandLength;
            this->_commandStatus=other._commandStatus;
            this->_sequenceNum=other._sequenceNum;
            this->_commandId=other._commandId;
        }
        virtual ~SMPPConnectResp();
    public:
        bool Pack(pack::Writer &writer) const;
		virtual UInt32 bodySize() const;
    public:

        std::string _systemId;
    };

} /* namespace smsp */
#endif /* SMPPCONNECTRESP_H_ */
