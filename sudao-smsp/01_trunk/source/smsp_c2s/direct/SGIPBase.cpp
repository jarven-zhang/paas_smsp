#include "SGIPBase.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "SnManager.h"

UInt32 SGIPBase::g_seqId = 0;
UInt32 SGIPBase::g_nNode = 0;

char * timestampSGIP(char *buf) 
{
	time_t tt;
	struct tm now = {0};

	time(&tt);
	localtime_r(&tt,&now);
	snprintf(buf,64,"%02d%02d%02d%02d%02d", now.tm_mon + 1,now.tm_mday,now.tm_hour,now.tm_min,now.tm_sec);
	return buf;
}

SGIPBase::SGIPBase() 
{
	char str[80];
	timestampSGIP(str);
	sequenceIdTime_ = atoi(str);
	packetLength_= 0;
	requestId_= 0;
	sequenceIdNode_ = g_nNode;
	sequenceId_ = 0;
}

SGIPBase::~SGIPBase()
{

}

bool SGIPBase::Pack(Writer& writer) const 
{
	writer(bodySize())(requestId_)(sequenceIdNode_)(sequenceIdTime_)(sequenceId_);
	return writer;
}

bool SGIPBase::Unpack(Reader& reader) 
{
	reader(packetLength_)(requestId_)(sequenceIdNode_)(sequenceIdTime_)(sequenceId_);
	return reader;

}

const char* SGIPBase::getCode(UInt32 code)
{
	
	switch (code) {
	case SGIP_BIND:
		return "SGIP_BIND";
		break;
	case SGIP_BIND_RESP:
		return "SGIP_BIND_RESP";
		break;
	case SGIP_UNBIND:
		return "SGIP_UNBIND";
		break;
	case SGIP_UNBIND_RESP:
		return "SGIP_UNBIND_RESP";
		break;
	case SGIP_SUBMIT:
		return "SGIP_SUBMIT";
		break;
	case SGIP_SUBMIT_RESP:
		return "SGIP_SUBMIT_RESP";
		break;
	case SGIP_REPORT:
		return "SGIP_REPORT";
		break;
	case SGIP_REPORT_RESP:
		return "SGIP_REPORT_RESP";
		break;
	
	default:
		return "CMPP_OTHER";
		break;
	}
	return "";

}

UInt32 SGIPBase::getSeqID()
{
	
	if(g_seqId >= 0xFFFFFFFE){
		g_seqId=0;
	}
	
	g_seqId++;
	return g_seqId;
}

