#include "SnManager.h"
#include <stdio.h>
#include <stdlib.h>


SnManager::SnManager()
{
    _sn   = 0;
	_sn64 = 0;
	_sn64Two = 0;
    m_uDirectSequenceId = 0;
	m_uNodeId = 0;
}

SnManager::~SnManager()
{

}
void SnManager::Init(UInt32 uNodeId)
{
	m_uNodeId = uNodeId;
}

UInt32 SnManager::getDirectSequenceId()
{
	UInt32 retval = 0;
	
	if(m_uNodeId + (m_uDirectSequenceId + 1)*100 > 0xFFFFFFFF)
	{
		m_uDirectSequenceId = 0;
	}

	retval = m_uNodeId + (m_uDirectSequenceId)*100;
	
	m_uDirectSequenceId++;
	
	return retval;
}

UInt64 SnManager::getSn()
{
    UInt32 timer;
    timer = time(NULL);

    if(_sn > 0x00000000000F423E)
    {
        _sn=0;
    }
    _sn++;

    char str[64] = {0};
    UInt64 sn = 0;
    snprintf(str, 64,"%010d%06d", timer, _sn);
    sn = atol(str);

    return sn;
}

UInt64 SnManager::getSeq64(int which)
{
	if(which == 1)
	{
		if(_sn64 > 0xFFFFFFFFFFFFFFFE )
	    {
	        _sn64 = 0;
	    }
	    
	    return _sn64++;
	}
	else if(which == 2)
	{
		if(_sn64Two > 0xFFFFFFFFFFFFFFFE )
	    {
	        _sn64Two = 0;
	    }
	    
	    return _sn64Two++;
	}

	return 0;
}

void SnManager::setSeq64(int which, UInt64 start)
{
	if(which == 1)
	{
		_sn64 = start;
	}
	else if(which == 2)
	{
		_sn64Two = start;
	}
}

UInt64 SnManager::CreateSubmitId(UInt64 SwitchNumber)
{
	UInt64 submitId = 0;
	
/*	bit63~bit60:month
	bit59~bit55:day
	bit54~bit50:hour
	bit49~bit44:min
	bit43~bit38:second
	bit37~bit16:sm switch number
	bit15~bit0:sequence  */
	
	time_t tt;
	struct tm now = {0};
	time(&tt);
	localtime_r(&tt,&now);
	UInt64 mon, day, hour, min, second, sequence;

	if(_sn16 > 0xFFFE)
    {
        _sn16=0;
    }
    _sn16++;

	mon  = now.tm_mon + 1;
	day  = now.tm_mday;
	hour = now.tm_hour;
	min  = now.tm_min;
	second = now.tm_sec;
	sequence = _sn16;

	submitId |= (mon << 60);
	submitId |= (day << 55);
	submitId |= (hour<< 50);
	submitId |= (min << 44);
	submitId |= (second<<38);
	submitId |= (SwitchNumber<<16);		//load from config.xml
	submitId |=  sequence;
	
    return submitId;
}

UInt64 SnManager::CreateMoId(UInt64 SwitchNumber)
{
	UInt64 submitId = 0;
	
/*	bit63~bit60:month
	bit59~bit55:day
	bit54~bit50:hour
	bit49~bit44:min
	bit43~bit38:second
	bit37~bit16:sm switch number
	bit15~bit0:sequence  */
	
	time_t tt;
	struct tm now = {0};
	time(&tt);
	localtime_r(&tt,&now);
	UInt64 mon, day, hour, min, second, sequence;

	if(_sn16Mo > 0xFFFE)
    {
        _sn16Mo=0;
    }
    _sn16Mo++;

	mon  = now.tm_mon + 1;
	day  = now.tm_mday;
	hour = now.tm_hour;
	min  = now.tm_min;
	second = now.tm_sec;
	sequence = _sn16Mo;

	submitId |= (mon << 60);
	submitId |= (day << 55);
	submitId |= (hour<< 50);
	submitId |= (min << 44);
	submitId |= (second<<38);
	submitId |= (SwitchNumber<<16);		//load from config.xml
	submitId |=  sequence;
	
    return submitId;
}



