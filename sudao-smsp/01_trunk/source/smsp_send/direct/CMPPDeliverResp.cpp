#include "CMPPDeliverResp.h"
#include <arpa/inet.h>
#include "endianEx.h"
#include "main.h"

namespace smsp
{

    CMPPDeliverResp::CMPPDeliverResp()
    {
        _msgId = 0;
        _tpPId = 0;
        _tpUdhi = 0;
        _msgFmt = 0;
        _registeredDelivery = 0;
        _msgLength = 0;
        _reserved = 0;
        _status=4;
    }

    CMPPDeliverResp::~CMPPDeliverResp()
    {

    }

    bool CMPPDeliverResp::Unpack(Reader& reader)
    {
        reader(_msgId)(21, _destId)(10, _serviceId)(_tpPId)(_tpUdhi)(_msgFmt)(21,_srcTerminalId)(_registeredDelivery)(_msgLength);   

        /////LogDebug("cmpp reportFlag[%d],source[%s],dest[%s].",_registeredDelivery,_srcTerminalId.data(),_destId.data());
        if( _registeredDelivery == 1 && reader && _msgLength == 60 )
        {
        	reader((UInt32) _msgLength, _msgContent)(_reserved);
            /////LogDebug("CMPP Report content[%s].",_msgContent.data());
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
		else if(reader && (0 == _registeredDelivery) && (1 == _tpUdhi))
		{
			UInt8 tmp1, tmp2;
			reader(_longSmProtocalHeadType);
			UInt32 uMsgLenth = _msgLength - 1;
			if(_longSmProtocalHeadType == 5)
			{
				reader(tmp1)(tmp2)(_randCode8);
				_randCode = (UInt16)_randCode8;
				reader(_pkTotal)(_pkNumber);
				
				uMsgLenth = _msgLength - 6;
			}
			else if(_longSmProtocalHeadType == 6)
			{
				reader(tmp1)(tmp2)(_randCode);
				reader(_pkTotal)(_pkNumber);
				uMsgLenth = _msgLength - 7;
			}
			reader((UInt32) uMsgLenth, _msgContent)(_reserved);
		}
		else
		{
			reader((UInt32) _msgLength, _msgContent)(_reserved);
		}
		//add by fangjinxiong 20161117.
		int ureaded = 8+ 21 + 10 + 1 +1 +1 +21 +1 +1 + _msgLength + 8;
		int ileftByte = _totalLength - ureaded - CMPPBase::bodySize();
		if(ileftByte > 0)
		{
			LogDebug("ileftByte[%d]", ileftByte);
			string strtmp;
			reader((UInt32)ileftByte, strtmp);
		}
		//add end

		
        return reader;
    }

} /* namespace smsp */
