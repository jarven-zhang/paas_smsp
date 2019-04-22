#ifndef SMPPSUBMITREQH
#define SMPPSUBMITREQH
#include "SMPPBase.h"
namespace smsp
{

    class SMPPSubmitReq: public SMPPBase
    {
    public:
        SMPPSubmitReq();
        virtual ~SMPPSubmitReq();

		SMPPSubmitReq(const SMPPBase& other)
        {
            this->_commandLength=other._commandLength;
            this->_commandStatus=other._commandStatus;
            this->_sequenceNum=other._sequenceNum;
            this->_commandId=other._commandId;
        }
    public:
        virtual bool Unpack(pack::Reader &reader);
		bool readCOctetString(UInt32& position, UInt32 max, char* data, bool isBreak = false);
    public:

		std::string _buffer;
        std::string _sourceAddr;  ///sp
        UInt8 _esmClass;
		UInt8 _dataCoding;

        std::string _serviceType;
        UInt8 _sourceAddrTon;
        UInt8 _sourceAddrNpi;


        UInt8 _destAddrTon;
        UInt8 _destAddrNpi;
        std::string _destinationAddr;   ////phone


        UInt8 _protocolId;
        UInt8 _priorityFlag;
        std::string _scheduleDeliveryTime;
        std::string _validityPeriod;
        UInt8 _registeredDelivery;
        UInt8 _replaceIfPresentFlag;
        
        UInt8 _smDefaultMsgId;

        UInt8 _smLength;
        std::string _shortMessage;

		UInt8 _pkTotal;
        UInt8 _pkNumber;
		UInt8 _randeCode;

		std::string _phone; ////destinationAddr
		UInt8 _phoneCount;
		std::vector<std::string> _phonelist;

    };

} /* namespace smsp */
#endif /* SMPPSUBMITREQH */
