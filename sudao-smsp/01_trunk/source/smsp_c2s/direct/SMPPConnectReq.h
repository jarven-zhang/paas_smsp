#ifndef SMPPCONNECTREQ_H_
#define SMPPCONNECTREQ_H_
#include "SMPPBase.h"
namespace smsp
{

    class SMPPConnectReq: public SMPPBase
    {
    public:
        SMPPConnectReq();
        virtual ~SMPPConnectReq();
        SMPPConnectReq(const SMPPBase& other)
        {
            this->_commandLength=other._commandLength;
            this->_commandStatus=other._commandStatus;
            this->_sequenceNum=other._sequenceNum;
            this->_commandId=other._commandId;
        }
        
    public:
        bool readCOctetString(UInt32& position, UInt32 max, char* data);
        virtual bool Unpack(pack::Reader &reader);

    public:
// 16,9,13,1,1,1,41
		std::string _buffer;

        std::string _systemId;
        std::string _password;
        std::string _systemType;
        UInt8 _version;
        UInt8 _addrTon;
        UInt8 _addrNpi;
        std::string _addrRange;

    };

} /* namespace smsp */
#endif /* SMPPCONNECTREQ_H_ */
