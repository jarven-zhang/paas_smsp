#ifndef CMPP3DELIVERREQ_H_
#define CMPP3DELIVERREQ_H_
#include "CMPP3Base.h"
namespace cmpp3
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
        UInt64 _msgId;
        std::string _destId;
        std::string _serviceId;
        UInt8 _tpPId;
        UInt8 _tpUdhi;
        UInt8 _msgFmt;
        std::string _srcTerminalId;
		UInt8 _srcTerminalType;
        UInt8 _registeredDelivery;
        UInt8 _msgLength;
		UInt32 _SMSCSequence;
		UInt8 _longSmProtocalHeadType;
		UInt8 _randCode8;   
		UInt16 _randCode;
		UInt8 _pkTotal;
        UInt8 _pkNumber;
        std::string _msgContent;
		std::string _submitTime;
		std::string _doneTime;
		std::string _destTerminalId;
        UInt32 _status;
        std::string _remark;
		std::string _linkID;


    };

} /* namespace smsp */
#endif /* CMPPDELIVERREQ_H_ */
