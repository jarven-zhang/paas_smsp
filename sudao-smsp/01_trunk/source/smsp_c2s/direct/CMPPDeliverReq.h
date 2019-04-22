#ifndef CMPPDELIVERREQ_H_
#define CMPPDELIVERREQ_H_
#include "CMPPBase.h"
namespace smsp
{

    class CMPPDeliverReq: public CMPPBase
    {
    public:
        CMPPDeliverReq();
        virtual ~CMPPDeliverReq();
    public:
        virtual bool Unpack(Reader &reader);
		virtual bool Pack(Writer &writer) const;
		virtual UInt32 bodySize() const;

		void setContent(char* content,UInt32 len)
        {
            _msgContent.reserve(20480);
            _msgContent.append(content,len);
            _msgLength = len;
        }
		
    public:
        // 8 21 10 1 1 1 21 1 1 len 8
        UInt64 _msgId;
        std::string _destId;
        std::string _serviceId;
        UInt8 _tpPId;
        UInt8 _tpUdhi;
        UInt8 _msgFmt;
        std::string _srcTerminalId;
        UInt8 _registeredDelivery;
        UInt8 _msgLength;
        std::string _msgContent;
        UInt64 _reserved;

        UInt32 _status;
        std::string _remark;
		
		UInt32 _SMSCSequence;
		std::string _reportTime;



    };

} /* namespace smsp */
#endif /* CMPPDELIVERREQ_H_ */
