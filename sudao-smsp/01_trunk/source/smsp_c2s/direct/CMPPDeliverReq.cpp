#include "CMPPDeliverReq.h"
#include <arpa/inet.h>
#include "endianEx.h"
#include "main.h"

namespace smsp
{
    CMPPDeliverReq::CMPPDeliverReq()
    {
        _msgId = 0;
        _tpPId = 0;
        _tpUdhi = 0;
        _msgFmt = 0;
        _registeredDelivery = 0;
        _msgLength = 8 + 7 + 10 + 10 + 21 + 4;
        _reserved = 0;
        _status=4;
		_commandId = CMPP_DELIVER;
        _SMSCSequence = 0;
    }

    CMPPDeliverReq::~CMPPDeliverReq()
    {

    }

	UInt32 CMPPDeliverReq::bodySize() const
    {
        if (1 == _registeredDelivery)
        {
            return 8 + 21 + 10 + 1 + 1 + 1 + 21 + 1 + 1 + 60 + 8 + CMPPBase::bodySize();
        }
        else
        {
            return 8 + 21 + 10 + 1 + 1 + 1 + 21 + 1 + 1 + _msgLength + 8 + CMPPBase::bodySize();
        }
    }

	bool CMPPDeliverReq::Pack(Writer &writer) const
	{
        char destId[22] = {0};
        snprintf(destId,22,"%s",_destId.data());

        char serviceId[11] = {0};
        snprintf(serviceId,11,"%s",_serviceId.data());

        char srcTerminalId[22] = {0};
        snprintf(srcTerminalId,22,"%s",_srcTerminalId.data());

        char reMark[8] = {0};
        snprintf(reMark,8,"%s",_remark.data());
        
        
        if (1 == _registeredDelivery) ////report
        {
            char timesec[25] = {0};
            timeYear(timesec);

            ////LogNotice("************msgid[%lu].",_msgId);
            
            CMPPBase::Pack(writer);
            writer(_msgId)((UInt32)21, destId)((UInt32)10, serviceId)(_tpPId)(_tpUdhi)(_msgFmt)((UInt32)21,srcTerminalId)
                (_registeredDelivery)(_msgLength)(_msgId)((UInt32)7, reMark)((UInt32)10, timesec)((UInt32)10, timesec)
                ((UInt32)21, srcTerminalId)(_SMSCSequence)(_reserved);
        }
        else ////up
        {
            CMPPBase::Pack(writer);
            writer(_msgId)((UInt32)21, destId)((UInt32)10, serviceId)(_tpPId)(_tpUdhi)(_msgFmt)((UInt32)21,srcTerminalId)
                (_registeredDelivery)(_msgLength)((UInt32) _msgLength, _msgContent.data())(_reserved);
        }
		return writer;
	}

    bool CMPPDeliverReq::Unpack(Reader& reader)
    {
        reader(_msgId)(21, _destId)(10, _serviceId)(_tpPId)(_tpUdhi)(_msgFmt)(21,_srcTerminalId)(_registeredDelivery)(_msgLength);
        reader((UInt32) _msgLength, _msgContent)(_reserved);
        if(reader && _msgLength == 60)
        {
            UInt64 msgId;
            char stat[8]= {0};
            const char* pos=_msgContent.data();
            memcpy(&msgId,pos,sizeof(msgId));
            pos+=sizeof(msgId);
            memcpy(stat,pos,7);
            msgId = pack::Endian::FromBig(msgId);
            if(strcmp(stat,"DELIVRD")== 0)
            {
                _status=0;
            }
            _remark= stat;
            _msgId=msgId;
        }
        return reader;
    }

} /* namespace smsp */
