#include "SGIPDeliverReq.h"
#include <stdio.h>
#include "UTFString.h"
#include "main.h"


SGIPDeliverReq::SGIPDeliverReq()
{
    m_strMsgContent = "";
    m_strPhone = "";
    m_strReserve = "";
    m_strSpNum = "";
    m_uMsgCoding = 0;
    m_uMsgLength = 0;
    m_uTpPid = 0;
    m_uTpUdhi = 0;
}

SGIPDeliverReq::~SGIPDeliverReq()
{

}

bool SGIPDeliverReq::Unpack(Reader &reader)
{
    reader(21,m_strPhone)(21, m_strSpNum)(m_uTpPid)(m_uTpUdhi)(m_uMsgCoding)(m_uMsgLength);
    reader((UInt32) m_uMsgLength, m_strMsgContent)(8,m_strReserve);

	//add by fangjinxiong 20161117.    21+21+1+1+1+4+8 
	int ileftByte = packetLength_ -21 -21 -1 -1 -1 -4 -(UInt32)m_uMsgLength -8 - SGIPBase::bodySize();	//rewrite bodySize
	if(ileftByte > 0)
	{
		LogDebug("ileftByte[%d]", ileftByte);
		string strtmp;
		reader((UInt32)ileftByte, strtmp);
	}
	//add end
	
    return reader;
}