
#include "SMGPSubmitResp.h"
#include <stdio.h>

namespace smsp
{
	SMGPSubmitResp::SMGPSubmitResp()
	{
		msgId_="";
		status_=0;
		requestId_ = SMGP_SUBMIT_RESP;
		m_uSubmitId = 0;
	}

	SMGPSubmitResp::~SMGPSubmitResp()
	{
	}
	
	UInt32 SMGPSubmitResp::bodySize() const 
	{
		return 10 + 4 + SMGPBase::bodySize();
	}

	bool SMGPSubmitResp::Unpack(Reader& reader)
	{
		msgId_.reserve(11);
		char msgID[10]={0};
		char msg[30]={0};
		reader((UInt32)10,msgID)(status_);
		char* pos=msg;
		for(int i=0;i<10;i++)
	    {
			snprintf(pos,30,"%02x",msgID[i]);
			pos+=2;
		}
		msgId_=msg;
		return reader;
	}
	
	bool SMGPSubmitResp::Pack(Writer& writer) const
    {
		unsigned char cMsgId[11] = {0};
		memcpy(cMsgId,(unsigned char*)&m_uSubmitId,8);

		
    	SMGPBase::Pack(writer);
        writer((UInt32)10,(char*)cMsgId)(status_);
        return writer;
    }
}
