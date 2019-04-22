#include "CoverRate.h"
#include "main.h"


CoverRate::CoverRate()
{
    m_mapOverRate.clear();
}
CoverRate:: ~CoverRate()
{

}


////OverRate is bool, not OverRate is false
bool CoverRate::isOverRate(std::string& strData)
{
    //strData[SASP_OverRate:accountID&phone&m_strSmsType]
    //strData[SASP_OverRate:accountID&phone&m_strSmsType]
    Int64 curTime = time(NULL);

    std::map<std::string,long>::iterator iter = m_mapOverRate.begin();
    for (; iter != m_mapOverRate.end(); ++iter)
    {
        if (0 == strData.compare(iter->first))
        {
            if (curTime <= iter->second)
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
    }

    //LogDebug("fjx:m_mapOverRate.size[%d]", m_mapOverRate.size());
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


void CoverRate::cleanOverRateMap()
{
    if(m_mapOverRate.size() > 0)
        m_mapOverRate.clear();
}


