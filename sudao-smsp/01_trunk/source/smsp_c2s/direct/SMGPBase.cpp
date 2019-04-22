#include "SMGPBase.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "SnManager.h"


namespace smsp
{
	UInt32 SMGPBase::g_seqId = 0;

/*
    char * timestamp(char *buf)
    {
        time_t tt;
        struct tm *now;

        time(&tt);
        now = localtime(&tt);
        snprintf(buf, 64,"%02d%02d%02d%02d%02d", now->tm_mon + 1, now->tm_mday,
                now->tm_hour, now->tm_min, now->tm_sec);
        return buf;
    }
*/

	SMGPBase::SMGPBase() 
	{
		packetLength_ = 0;
		requestId_ = 0;
		sequenceId_ = 0;
	}

	SMGPBase::~SMGPBase()
	{

	}

	bool SMGPBase::Pack(Writer& writer) const
	{
		writer(bodySize())(requestId_)(sequenceId_);
		return writer;
	}

	bool SMGPBase::Unpack(Reader& reader)
	{
		reader(packetLength_)(requestId_)(sequenceId_);
		return reader;

	}
	const char* SMGPBase::getCode(UInt32 code)
	{

		switch (code) {
		case SMGP_LOGIN:
			return "SMGP_LOGIN";
			break;
		case SMGP_LOGIN_RESP:
			return "SMGP_LOGIN_RESP";
			break;
		case SMGP_SUBMIT:
			return "SMGP_SUBMIT";
			break;
		case SMGP_SUBMIT_RESP:
			return "SMGP_SUBMIT_RESP";
			break;
		case SMGP_ACTIVE_TEST:
			return "SMGP_ACTIVE_TEST";
			break;
		case SMGP_ACTIVE_TEST_RESP:
			return "SMGP_ACTIVE_TEST_RESP";
			break;
		case SMGP_TERMINATE:
			return "SMGP_TERMINATE";
			break;
		case SMGP_TERMINATE_RESP:
			return "SMGP_TERMINATE_RESP";
			break;
			/*
		case SMGP_QUERY:
			return "SMGP_QUERY";
			break;
		case SMGP_QUERY_RESP:
			return "SMGP_QUERY_RESP";
			break;*/
		case SMGP_DELIVER:
			return "SMGP_DELIVER";
			break;
		case SMGP_DELIVER_RESP:
			return "SMGP_DELIVER_RESP";
			break;
		default:
			return "";
			break;
		}
		
		return "";
	}


	UInt32 SMGPBase::getSeqID()
	{
		if(g_seqId >= 0xFFFFFFFE)
	    {
			g_seqId=0;
		}

		g_seqId++;
		return g_seqId;
	}
}

