#include "CMPP3DeliverReq.h"
#include <arpa/inet.h>
#include "endianEx.h"
#include "main.h"

namespace cmpp3
{
    CMPPDeliverReq::CMPPDeliverReq()
    {
        _msgId = 0;
        _tpPId = 0;
        _tpUdhi = 0;
        _msgFmt = 0;
        _registeredDelivery = 0;
        _msgLength = 8 + 7 + 10 + 10 + 32 + 4;
		_srcTerminalType = 0;
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
            return 8 + 21 + 10 + 1 + 1 + 1 + 32 + 1 + 1 + 1 + 71 + 20 + CMPPBase::bodySize();
        }
        else
        {
            return 8 + 21 + 10 + 1 + 1 + 1 + 32 + 1 + 1 + 1 + _msgLength + 20 + CMPPBase::bodySize();
        }
    }

	bool CMPPDeliverReq::Pack(Writer &writer) const
	{
        char destId[22] = {0};
        snprintf(destId,22,"%s",_destId.data());

        char serviceId[11] = {0};
        snprintf(serviceId,11,"%s",_serviceId.data());

        char srcTerminalId[33] = {0};
        snprintf(srcTerminalId,33,"%s",_srcTerminalId.data());

        char reMark[8] = {0};
        snprintf(reMark,8,"%s",_remark.data());
        
        if (1 == _registeredDelivery) ////report
        {
            char timesec[25] = {0};
            timeYear(timesec);

            ////LogNotice("************msgid[%lu].",_msgId);
            
            CMPPBase::Pack(writer);
            writer(_msgId)((UInt32)21, destId)((UInt32)10, serviceId)(_tpPId)(_tpUdhi)(_msgFmt)((UInt32)32,srcTerminalId)
                (_srcTerminalType)(_registeredDelivery)(_msgLength)(_msgId)((UInt32)7, reMark)((UInt32)10, timesec)
                ((UInt32)10, timesec)((UInt32)32, srcTerminalId)(_SMSCSequence)(20,_linkID);
        }
        else ////up
        {
            CMPPBase::Pack(writer);
            writer(_msgId)((UInt32)21, destId)((UInt32)10, serviceId)(_tpPId)(_tpUdhi)(_msgFmt)((UInt32)32,srcTerminalId)
                (_srcTerminalType)(_registeredDelivery)(_msgLength)((UInt32) _msgLength, _msgContent.data())(20,_linkID);
        }
		return writer;
	}

    bool CMPPDeliverReq::Unpack(Reader& reader)
    {
		reader(_msgId)(21, _destId)(10, _serviceId)(_tpPId)(_tpUdhi)(_msgFmt)(32,_srcTerminalId)
				(_srcTerminalType)(_registeredDelivery)(_msgLength);
	   
        /////LogDebug("cmpp reportFlag[%d],source[%s],dest[%s].",_registeredDelivery,_srcTerminalId.data(),_destId.data());
        if(reader && 1 == _registeredDelivery && _msgLength == 71)//report
        {
        	reader((UInt32) _msgLength, _msgContent)(20,_linkID);
            UInt64 msgId;
            char stat[8]= {0};
			char destTerminalID[33] = {0};
			char strtime[11] = {0};
			UInt32 SMSCSequence;
            const char* pos=_msgContent.data();
			//Msg_Id
            memcpy(&msgId,pos,sizeof(msgId));
            pos+=sizeof(msgId);
		    msgId = pack::Endian::FromBig(msgId);
			//Stat
            memcpy(stat,pos,7);
			pos += 7;
			
			//Submit_time
			memcpy(strtime,pos,10);
			pos += 10;
			
			//Done_time
			memset(strtime,0,sizeof(strtime));
			memcpy(strtime,pos,10);
			pos += 10;
			
			//Dest_terminal_Id
			memcpy(destTerminalID,pos,32);
			pos += 32;
			
			//SMSC_sequence
            memcpy(&SMSCSequence,pos,sizeof(SMSCSequence));
			SMSCSequence = pack::Endian::FromBig(SMSCSequence);
            if(strcmp(stat,"DELIVRD")== 0)
            {
                _status=0;
            }
            _remark = stat;
            _msgId = msgId;
			_submitTime = strtime;
			_doneTime = strtime;
			_destTerminalId = destTerminalID;
			_SMSCSequence = SMSCSequence;
        }
		else if(reader && (0 == _registeredDelivery) && (1 == _tpUdhi))
		{//long mo
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
			reader((UInt32) uMsgLenth, _msgContent)(20,_linkID);
		}
		else
		{
			reader((UInt32) _msgLength, _msgContent)(20,_linkID);
		}

		//add by fangjinxiong 20161117.
		int ureaded = 8+ 21 + 10 + 1 +1 +1 +32 +1 +1 +1 + _msgLength + 20;
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
