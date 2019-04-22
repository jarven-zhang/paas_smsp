#include "CoverRate.h"

CoverRate::CoverRate()
{
    m_mapOverRate.clear();
	m_mapKeyWordOverRate.clear();
}
CoverRate:: ~CoverRate()
{

}

////OverRate is bool, not OverRate is false
bool CoverRate::isOverRate(std::string& strData)
{
    Int64 curTime = time(NULL);

	std::map< std::string,long>::iterator iter = m_mapOverRate.find( strData );
	if( iter != m_mapOverRate.end())
	{
		if ( curTime <= iter->second )
		{
			Int64 time_ = iter->second;
			struct tm tblock = {0};
			localtime_r(&time_,&tblock);
			LogWarn("key[%s] is OverRate hold up, end time[%s].", strData.data(), asctime(&tblock));
			return true;
		}
		else
		{
			LogWarn("key[%s] OverRate hold up is overdue, curtime[%d], end time[%d]", strData.data(), curTime, iter->second);
			m_mapOverRate.erase(iter);
			return false;
		}
	}

    //LogDebug("fjx:m_mapOverRate.size[%d]", m_mapOverRate.size());
    return false;
}

bool CoverRate::isKeyWordOverRate(std::string& strData)
{
    Int64 curTime = time(NULL);
	std::map<std::string,long>::iterator iter = m_mapKeyWordOverRate.find( strData );

	/*看内存是否存在超频记录*/
	if( iter != m_mapKeyWordOverRate.end())
	{
		if ( curTime <= iter->second )
		{
			Int64 time_ = iter->second;
			struct tm tblock = {0};
			localtime_r(&time_,&tblock);
			LogWarn("key[%s] is Key word OverRate hold up, end time[%s].",
				    strData.data(), asctime(&tblock));
			return true;
		}
		else
		{
			LogWarn("key[%s] Key word OverRate hold up is overdue, curtime[%d], end time[%d]",
				    strData.data(), curTime, iter->second);
			m_mapKeyWordOverRate.erase(iter);
			return false;
		}
	}


    return false;

}

bool CoverRate::isChannelOverRate(std::string& strData, models::TempRule &TempRule)
{
    //strData[SASP_Channel_OverRate:channelid&smstype&sign&Phone]
    LogDebug("strData[%s]", strData.data());
    Int64 curTime = time(NULL);
	UInt32 total_count = 0;
	Comm comm;
	//day overrate
	string channelDayData = "SASP_Channel_Day_OverRate:" + strData;
	channelOverrateMapType::iterator iterDay = m_mapChannelDayOverRate.find( channelDayData );
	if (iterDay != m_mapChannelDayOverRate.end())
	{
		string strCurDate = comm.getCurrentDay();
		strCurDate = strCurDate.substr(0, 8);
		strCurDate.append("00:00:00");

		Int64 curTimestamp = comm.strToTime( const_cast< char* > ( strCurDate.data()));
		LogDebug("CurTime[ %s, %ld, %ld ]", strCurDate.data(), curTimestamp, curTime);

		STL_MAP_STR::iterator iterDay1 = iterDay->second.begin();
		for (; iterDay1 != iterDay->second.end(); iterDay1++)
		{
			string strDate = iterDay1->first; 
			strDate.append("00:00:00");
			Int64 uRedisStamp = comm.strToTime(const_cast<char *>(strDate.data())); 
			Int64 uDiff = curTimestamp - uRedisStamp ;

			if( uDiff >=  7 * 24 * 3600) 
			{
				//do nothing
			}			
			else
			{
				if((TempRule.m_overRate_time_h >= 24) && uDiff <= ( TempRule.m_overRate_time_h - 24 )* 3600 )
				{
					total_count += atoi(iterDay1->second.data());
				}
			}

		}

		if( total_count >= TempRule.m_overRate_num_h )
		{
			LogWarn("[%s]channel_overRate warn, total sended: %u! num_h[%u], time_h[%u]", channelDayData.data(),
	    			total_count, TempRule.m_overRate_num_h, TempRule.m_overRate_time_h);		
			return true;
		}	
		else
		{
			LogDebug("[%s]do not reach channel_overRate, total sended: %u! num_h[%u], time_h[%u]", channelDayData.data(),
						total_count, TempRule.m_overRate_num_h, TempRule.m_overRate_time_h);
			m_mapChannelDayOverRate.erase(iterDay);		
		}
	}			
	
	//minutes seconds overrate
	string channelData = "SASP_Channel_OverRate:" + strData;
	std::map< std::string,long>::iterator iter = m_mapChannelOverRate.find( channelData );
	if( iter != m_mapChannelOverRate.end())
	{
		if ( curTime <= iter->second )
		{
			Int64 time_ = iter->second;
			struct tm tblock = {0};
			localtime_r(&time_,&tblock);
			LogWarn("key[%s] is channel overRate hold up, end time[%s].", channelData.data(), asctime(&tblock));
			return true;
		}
		else
		{
			LogWarn("key[%s] channel overRate hold up is overdue, curtime[%d], end time[%d]", channelData.data(), curTime, iter->second);
			m_mapChannelOverRate.erase(iter);
			return false;
		}
	}
    
    return false;
	
}


void CoverRate::setOverRatePer(std::string& strKey, long Time)
{
    //sendrecord:accountid&phone&tempid
    if (strKey.empty())
    {
        LogError("[CoverRate::setOverRatePer] strData is empty.");
        return;
    }

    LogWarn("[CoverRate::setOverRatePer] OverRate key[%s] end time[%lld]", strKey.data(),Time);

    //pthread_mutex_lock(&m_mutt);
    m_mapOverRate[strKey] = Time;
    //pthread_mutex_unlock(&m_mutt);
    return;
}

void CoverRate::setKeyWordOverRatePer(std::string& strKey, long Time)
{
    //sendrecord:accountid&phone&tempid
    if (strKey.empty())
    {
        LogError("[CoverRate::setKeyWordOverRatePer] strData is empty.");
        return;
    }

    LogWarn("[CoverRate::setKeyWordOverRatePer] OverRate key[%s] end time[%lld]", strKey.data(),Time);

    //pthread_mutex_lock(&m_mutt);
    m_mapKeyWordOverRate[strKey] = Time;
    //pthread_mutex_unlock(&m_mutt);
    return;
}

void CoverRate::setChannelOverRatePer(std::string& strKey, long Time)
{
    //sendrecord:accountid&phone&tempid
    if (strKey.empty())
    {
        LogError("[CoverRate::setChannelOverRatePer] strData is empty.");
        return;
    }

    LogWarn("[CoverRate::setChannelOverRatePer] channel overRate key[%s] end time[%lld]", strKey.data(),Time);
	
	string key = "SASP_Channel_OverRate:" + strKey;
    m_mapChannelOverRate[key] = Time;
	LogDebug("setChannelOverRatePer:key=%s", key.data());
    return;
}

void CoverRate::setChannelDayOverRatePer(std::string& strKey, STL_MAP_STR channelDayDataMap)
{
    if (strKey.empty())
    {
        LogError("[CoverRate::setChannelOverRatePer] strData is empty.");
        return;
    }

    LogWarn("[CoverRate::setChannelDayOverRatePer] channel overRate key[%s]", strKey.data());
	
	string key = "SASP_Channel_Day_OverRate:" + strKey;
    m_mapChannelDayOverRate[key] = channelDayDataMap;
	LogDebug("setChannelDayOverRatePer:key=%s", key.data());
    return;
}

void CoverRate::cleanOverRateMap()
{
    if(m_mapOverRate.size() > 0)
        m_mapOverRate.clear();
}

void CoverRate::cleanChannelOverRateMap()
{
    if (m_mapChannelOverRate.size() > 0)
        m_mapChannelOverRate.clear();
	
	if (m_mapChannelDayOverRate.size() > 0)
		m_mapChannelDayOverRate.clear();
}

void CoverRate::cleanKeyWordOverRateMap()
{
    if(m_mapKeyWordOverRate.size() > 0)
        m_mapKeyWordOverRate.clear();
}


