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
            this->_commandId=other._commandId;
        }

    public:
        bool readCOctetString(UInt32& position, UInt32 max, char* data);
        virtual bool Unpack(pack::Reader &reader);
    public:

        std::string _buffer;

        std::string _sourceAddr;
        std::string _serviceType;
        UInt8 _sourceAddrTon;
        UInt8 _sourceAddrNpi;


        UInt8 _destAddrTon;
        UInt8 _destAddrNpi;
        std::string _destinationAddr;

        UInt8 _esmClass;
        UInt8 _protocolId;
        UInt8 _priorityFlag;
        std::string _scheduleDeliveryTime;
        std::string _validityPeriod;
        UInt8 _registeredDelivery;
        UInt8 _replaceIfPresentFlag;
        UInt8 _dataCoding;
        UInt8 _smDefaultMsgId;
        UInt8 _smLength;
        std::string _shortMessage;

        std::string _msgId;
        std::string _remark;
        UInt32 _status;


    };

} /* namespace smsp */
#endif /* SMPPDELIVERRESP_H_ */
