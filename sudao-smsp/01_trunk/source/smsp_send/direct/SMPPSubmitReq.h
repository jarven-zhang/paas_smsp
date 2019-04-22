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
    public:
        virtual UInt32 bodySize() const;
        virtual bool Pack(pack::Writer &writer) const;
        virtual bool Unpack(pack::Reader &reader);
    public:
        void setContent(std::string content,UInt32 len)
        {
            _shortMessage.reserve(20480);
            const char *src = content.data();
            _shortMessage.append(src,len);
            _smLength = len;
        }

        void setContent(char* content,UInt32 len)
        {
            _shortMessage.reserve(20480);
            _shortMessage.append(content,len);
            _smLength = len;
        }
        std::string _phone;
        std::string _sourceAddr;
        UInt8 _esmClass;
		UInt8 _dataCoding;
    private:
        std::string _serviceType;
        UInt8 _sourceAddrTon;
        UInt8 _sourceAddrNpi;


        UInt8 _destAddrTon;
        UInt8 _destAddrNpi;
        std::string _destinationAddr;


        UInt8 _protocolId;
        UInt8 _priorityFlag;
        std::string _scheduleDeliveryTime;
        std::string _validityPeriod;
        UInt8 _registeredDelivery;
        UInt8 _replaceIfPresentFlag;
        
        UInt8 _smDefaultMsgId;

        UInt8 _smLength;
        std::string _shortMessage;

    };

} /* namespace smsp */
#endif /* SMPPSUBMITREQH */
