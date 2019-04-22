#include "SMGPDeliverReq.h"
#include <stdio.h>
#include "main.h"

namespace smsp
{
	SMGPDeliverReq::SMGPDeliverReq()
	{
		_msgId = "";
		_isReport = 0;
		_msgFmt = 0;
		_recvTime = "";
		_srcTermId = "";
		_destTermId = "";

		_msgLength = 122;
		_msgContent = "";
		_reserve = "";
		_status = 4;
		_msgId.reserve(10);
		_recvTime.reserve(14);
		_srcTermId.reserve(21);
		_destTermId.reserve(21);
		_msgContent.reserve(1024);
		requestId_ = SMGP_DELIVER;

		m_uSubmitId = 0;
		_err = "";
	}

	SMGPDeliverReq::~SMGPDeliverReq()
	{

	}

	UInt32 SMGPDeliverReq::bodySize() const
    {
        if (1 == _isReport)
        {
            return 122 + 77 + 12;
        }
        else
        {
            return 10 +1 +1 + 14 + 21 + 21 +1  + _msgLength + 8 + SMGPBase::bodySize();
        }
    }

	bool SMGPDeliverReq::Pack(Writer &writer) const
	{
        ///char msgId[11] = {0};
	    ///snprintf(msgId,11,"%s",_msgId.data());

		unsigned char cMsgId[11] = {0};
		//unsigned char cMoMsgId[11] = {0};
		memcpy(cMsgId,(unsigned char*)&m_uSubmitId,8);
		//snprintf(cMsgId,11,"%s",_msgId.data());
		//LogDebug("msgId = %ld",m_uSubmitId);
		char recvTime[15 ] = {0};
		snprintf(recvTime,15,"%s",_recvTime.data());

		char srcTermId[22] = {0};
		snprintf(srcTermId,22,"%s",_srcTermId.data());

		char destTermId[22] = {0};
		snprintf(destTermId,22,"%s",_destTermId.data());

		/*char msgContent[_msgLength + 1] = {0};
		snprintf(msgContent,_msgLength,"%s",_msgContent.data());*/

		char reserve[9] = {0};
		snprintf(reserve,9,"%s",_reserve.data());

		char remark[8] = {0};
		snprintf(remark,8,"%s",_remark.data());

		
		char s_id[4] = {0};
		snprintf(s_id,4,"%s","id:");

		char s_sub[6] ={0};
		snprintf(s_sub,6,"%s"," sub:");

		char s_dlvrd[8] ={0};
		snprintf(s_dlvrd,8,"%s"," dlvrd:");

		char s_submit_data[14] ={0};
		snprintf(s_submit_data,14,"%s"," Submit_date:");

		char s_done_data[12] = {0};
		snprintf(s_done_data,12,"%s"," done_date:");

		char s_stat[7] = {0};
		snprintf(s_stat,7,"%s"," stat:");
		
		char s_err[6] = {0};
		snprintf(s_err,6,"%s"," err:");
		
		char s_text[7] ={0};
		snprintf(s_text,7,"%s"," Text:");
		
		
		char sub[4] = {'0','0','1','\0'};
		char dlvrd[4] = {'0','0','1','\0'};		
		char err[4] = {0};
		if(!_err.empty())
		{
			snprintf(err,4,"%s",_err.data());
		}
		char txt[21] = {0};
		/////id:III sub:001 dlvrd:001 Submit_date:1609212359 done_date:1609220000 stat:DELIVRD err:000 Text:
		
        if (1 == _isReport) ////report
        {
            char timesec[25] = {0};
            timeYear(timesec);     
			SMGPBase::Pack(writer);
			////LogDebug("===start pack ===length:[%d],commandId:[%d],sequenceId[%d]",bodySize(),requestId_,sequenceId_);
			writer((UInt32)10,(char*)cMsgId)(_isReport)(_msgFmt)((UInt32)14,recvTime)((UInt32)21,srcTermId)((UInt32)21,destTermId)
				(_msgLength)
				((UInt32)3,s_id)((UInt32)10,(char*)cMsgId)
				((UInt32)5,s_sub)((UInt32)3,sub)
				((UInt32)7,s_dlvrd)((UInt32)3,dlvrd)
				((UInt32)13,s_submit_data)((UInt32)10,timesec)
				((UInt32)11,s_done_data)((UInt32)10,timesec)	
				((UInt32)6,s_stat)((UInt32)7,remark)
				((UInt32)5,s_err)((UInt32)3,err)
				((UInt32)6,s_text)((UInt32)20,txt)
				((UInt32)8,reserve);

			
			/////LogDebug("====msgid:[%s],srcTermId:[%d],destTermId:[%d].",_msgId.data(),srcTermId,destTermId);
			
        }
        else ////up
        {
        	 char cMoMsgId[11] = {0};
        	snprintf(cMoMsgId,11,"%s",_msgId.data());
            SMGPBase::Pack(writer);
			writer((UInt32)10,(char*)cMoMsgId)(_isReport)(_msgFmt)((UInt32)14,recvTime)((UInt32)21,srcTermId)((UInt32)21,destTermId)
				(_msgLength)((UInt32)_msgLength,_msgContent.data())((UInt32)8,reserve) ; 
			/////LogDebug("=====content:[%s]",_msgContent.data());
        }
		return writer;
	}

	bool SMGPDeliverReq::Unpack(Reader& reader)
	{

		reader((UInt32) 10, _msgId)(_isReport)(_msgFmt)((UInt32) 14, _recvTime)(
				(UInt32) 21, _srcTermId)((UInt32) 21, _destTermId)(_msgLength);

		reader((UInt32) _msgLength, _msgContent)((UInt32) 8, _reserve);

	    LogDebug("smgp reportFlag[%d],source[%s],dest[%s].",_isReport,_srcTermId.data(),_destTermId.data());

		if (reader && _isReport == 1)
	    {
	        LogDebug("SMGP Report content[%s].",_msgContent.data());
			std::string stat;
			stat.reserve(20);
			std::string::size_type szPos = _msgContent.find("id:");
			std::string szMsgId;
			szMsgId.reserve(20);
			if (szPos != std::string::npos)
	        {
				szMsgId=_msgContent.substr(szPos+3,10);
			}

			szPos = _msgContent.find("stat:");
			if (szPos != std::string::npos)
	        {
				stat=_msgContent.substr(szPos+5,7);
			}
			char msg[30] = { 0 };
			char* posMsg = msg;
			for (int i = 0; i < 10; i++)
	        {
				snprintf(posMsg, 30,"%02x", szMsgId.data()[i]);
				posMsg += 2;
			}
			_msgId = msg;

			if (stat== "DELIVRD")
	        {
				_status = 0;
			}
			_remark=stat;
		}
		return reader;
	}
}
