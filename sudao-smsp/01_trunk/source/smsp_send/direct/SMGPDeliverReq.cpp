#include "SMGPDeliverReq.h"
#include <stdio.h>
#include "main.h"


SMGPDeliverReq::SMGPDeliverReq()
{
	_msgId = "";
	_isReport = 0;
	_msgFmt = 0;
	_recvTime = "";
	_srcTermId = "";
	_destTermId = "";

	_msgLength = 0;
	_msgContent = "";
	_reserve = "";
	_status = 4;
	_msgId.reserve(10);
	_recvTime.reserve(14);
	_srcTermId.reserve(21);
	_destTermId.reserve(21);
	_msgContent.reserve(1024);
}

SMGPDeliverReq::~SMGPDeliverReq()
{

}

bool SMGPDeliverReq::Unpack(Reader& reader)
{
	UInt32 ureaded = 0;	//has readed
	reader((UInt32) 10, _msgId)(_isReport)(_msgFmt)((UInt32) 14, _recvTime)(
			(UInt32) 21, _srcTermId)((UInt32) 21, _destTermId)(_msgLength);
	ureaded += (10 +1 +1 +14 +21 +21 +1);
	
	reader((UInt32) _msgLength, _msgContent)((UInt32) 8, _reserve);
	ureaded += ((UInt32)_msgLength + 8);

    LogDebug("smgp reportFlag[%d],source[%s],dest[%s].",_isReport,_srcTermId.data(),_destTermId.data());

	if (reader && _isReport == 1)
    {
        LogDebug("SMGP Report content[%s].",_msgContent.data());
		std::string stat;
		stat.reserve(20);
		std::string::size_type szPos = _msgContent.find("id:");
		if (szPos == std::string::npos)
		{
			szPos = _msgContent.find("Id:");
		}

		unsigned char uPSmsId[50] = {0};
		std::string szMsgId;
		szMsgId.reserve(20);
		if (szPos != std::string::npos)
        {
			szMsgId=_msgContent.substr(szPos+3,10);
		}
		memcpy((char*)uPSmsId,(char*)szMsgId.data(),10);

		szPos = _msgContent.find("stat:");
		if (szPos == std::string::npos)
		{
			szPos = _msgContent.find("Stat:");
		}
		
		if (szPos != std::string::npos)
        {
			stat=_msgContent.substr(szPos+5,7);
		}
		
		char msg[30] = { 0 };
		char* posMsg = msg;
		for (int i = 0; i < 10; i++)
        {
			snprintf(posMsg, 30,"%02x", uPSmsId[i]);
			posMsg += 2;
		}
		_msgId = msg;

		if (stat== "DELIVRD")
        {
			_status = 0;
		}

		/* BEGIN: Added by fxh, 2016/11/11   PN:RabbitMQ */
		szPos = _msgContent.find("err:");
		if (szPos == std::string::npos)
		{
			szPos = _msgContent.find("Err:");
		}
		
		if (szPos != std::string::npos && (_msgContent.substr(szPos).length())>=7)
		{
			m_strErrorCode = _msgContent.substr(szPos+4, 3);
		}
		/* END:   Added by fxh, 2016/11/11   PN:RabbitMQ */

		_remark=stat;
	}

	//add by fangjinxiong 20161117.
	int ileftByte = packetLength_ - ureaded - SMGPBase::bodySize();
	if(ileftByte > 0)
	{
		LogDebug("ileftByte[%d]", ileftByte);
		string strtmp;
		reader((UInt32)ileftByte, strtmp);
	}
	//add end
	
	return reader;
}