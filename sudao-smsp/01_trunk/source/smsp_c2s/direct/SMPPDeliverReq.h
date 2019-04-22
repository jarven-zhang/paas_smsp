#ifndef SMPPDELIVERREQ_H_
#define SMPPDELIVERREQ_H_
#include "SMPPBase.h"
namespace smsp
{

    class SMPPDeliverReq: public SMPPBase
    {
    public:
        SMPPDeliverReq();
        virtual ~SMPPDeliverReq();

        SMPPDeliverReq(const SMPPBase& other)
        {
            this->_commandLength=other._commandLength;
            this->_commandStatus=other._commandStatus;
            this->_sequenceNum=other._sequenceNum;
            this->_commandId=other._commandId;
        }

    public:
		virtual UInt32 bodySize() const;
        virtual bool Pack(pack::Writer &writer) const;

		void setContent(std::string content,UInt32 len)
        {
            _shortMessage.reserve(1024);
            const char *src = content.data();
            _shortMessage.append(src,len);
            _smLength = len;
        }

        void setContent(char* content,UInt32 len)
        {
            _shortMessage.reserve(1024);
            _shortMessage.append(content,len);
            _smLength = len;
        }
    public:
    	std::string _serviceType;
        UInt8 _sourceAddrTon;
        UInt8 _sourceAddrNpi;
		std::string _sourceAddr;
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

        std::string _msgId;  /////msgid
        std::string _remark; ////desc
        UInt32 _status;      ////status
    };
} /* namespace smsp */
#endif /* SMPPDELIVERRESP_H_ */

