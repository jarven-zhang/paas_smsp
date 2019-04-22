#ifndef CMPPDELIVERRESP_H_
#define CMPPDELIVERRESP_H_
#include "CMPPBase.h"
namespace smsp
{

    class CMPPDeliverResp: public CMPPBase
    {
    public:
        CMPPDeliverResp();
        virtual ~CMPPDeliverResp();
    public:
        virtual bool Unpack(Reader &reader);
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
		UInt8 _longSmProtocalHeadType;
		UInt8 _randCode8;   
		UInt16 _randCode;
		UInt8 _pkTotal;
        UInt8 _pkNumber;
        UInt8 _msgLength;		
        std::string _msgContent;
        UInt64 _reserved;

        UInt32 _status;
        std::string _remark;		

    };

} /* namespace smsp */
#endif /* CMPPDELIVERRESP_H_ */
