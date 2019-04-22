#include "ChannelSelect.h"
#include<iostream>
#include <algorithm>
#include "CRouterThread.h"
#include "main.h"

using namespace std;

#define PREFIXLEN 6

ChannelSelect::ChannelSelect()
{
    // OVERRATE default: 2/60;5/60;10/24;1
    m_TempRule.m_overRate_num_s         = 2;
    m_TempRule.m_overRate_time_s        = 60;
    m_TempRule.m_overRate_num_m         = 5;
    m_TempRule.m_overRate_time_m        = 60;
    m_TempRule.m_overRate_num_h         = 10;
    m_TempRule.m_overRate_time_h        = 24;
    m_TempRule.m_enable                 = 1;

    // KEYWORD_OVERRATE default: 1/60;3/60;4/24;1;1
    m_KeyWordTempRule.m_overRate_num_s  = 1;
    m_KeyWordTempRule.m_overRate_time_s = 60;
    m_KeyWordTempRule.m_overRate_num_m  = 3;
    m_KeyWordTempRule.m_overRate_time_m = 60;
    m_KeyWordTempRule.m_overRate_num_h  = 4;
    m_KeyWordTempRule.m_overRate_time_h = 24;
    m_KeyWordTempRule.m_enable          = 1;
    m_KeyWordTempRule.m_overrate_sign   = 1;

     // OVERRATE default: 2/60;5/60;10/24;1
    m_ChannelSysOverrateRule.m_overRate_num_s         = 2;
    m_ChannelSysOverrateRule.m_overRate_time_s        = 60;
    m_ChannelSysOverrateRule.m_overRate_num_m         = 5;
    m_ChannelSysOverrateRule.m_overRate_time_m        = 60;
    m_ChannelSysOverrateRule.m_overRate_num_h         = 10;
    m_ChannelSysOverrateRule.m_overRate_time_h        = 24;
    m_ChannelSysOverrateRule.m_enable                 = 0;

}

ChannelSelect::~ChannelSelect()
{

}

void ChannelSelect::initParam()
{
    m_userGwMap.clear();
    m_SignextnoGwMap.clear();
    m_SalePriceMap.clear();
    m_ChannelExtendPortTable.clear();

    g_pRuleLoadThread->getUserGwMap(m_userGwMap);
    g_pRuleLoadThread->getSignExtnoGwMap(m_SignextnoGwMap);
    g_pRuleLoadThread->getSmppPriceMap(m_PriceMap);
    g_pRuleLoadThread->getSmppSalePriceMap(m_SalePriceMap);
    g_pRuleLoadThread->getAllTempltRule(m_TempRuleMap); //m_TempRuleMap, KEY: userID+templetID
    g_pRuleLoadThread->getAllKeyWordTempltRule(m_KeyWordTempRuleMap);
    g_pRuleLoadThread->getChannelOverrateRule(m_ChannelOverrateMap);
    g_pRuleLoadThread->getPhoneAreaMap(m_PhoneAreaMap);
    g_pRuleLoadThread->getSysParamMap(m_SysParamMap);
    g_pRuleLoadThread->getChannelExtendPortTableMap(m_ChannelExtendPortTable);

    ParseGlobalOverRateSysPara(m_SysParamMap);
    ParseOverRateSysPara(m_SysParamMap);
    ParseKeyWordOverRateSysPara(m_SysParamMap);
    ParseChannelOverRateSysPara(m_SysParamMap);
}

bool ChannelSelect::getUserGw(string key, models::UserGw& userGw)       //KEY: userid_smstype
{
    if(!key.empty())
    {
        USER_GW_MAP::iterator it = m_userGwMap.find(key);
        if(it != m_userGwMap.end())
        {
            userGw = it->second;        //UserGw
        }
        else
        {
            LogDebug("[getUserGw] userid_smstype[%s] is not find.", key.data());
            return false;
        }

        return true;
    }
    else
    {
        LogDebug("channelSelect::getUserGw failed. userid_smsType=[%s]", key.data());
        return false;
    }
}

bool ChannelSelect::ParseGlobalOverRateSysPara(STL_MAP_STR sysParamMap)
{
    STL_MAP_STR::iterator map = sysParamMap.find("GLOBAL_OVERRATE");
    do{
        if(map == sysParamMap.end()){
            LogWarn("Can not find system parameter(GLOBAL_OVERRATE).");

            break;
        }

        std::string strtmp = map->second;
        LogDebug("overRate sysparma is[%s]", strtmp.data());
        std::string::size_type pos = strtmp.find("/");
        if(std::string::npos != pos  && strtmp.length() > pos){
            m_GloabalOverrateRule.m_overRate_num_h = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }

        pos = strtmp.find(";");
        if(std::string::npos != pos && strtmp.length() > pos){
            m_GloabalOverrateRule.m_overRate_time_h = atoi(strtmp.substr(0, pos).data()) * 24;
            strtmp = strtmp.substr(pos+1);
        }

        if(strtmp.length() == 1){
            m_GloabalOverrateRule.m_enable = atoi(strtmp.data());
        }
    }
    while (0);
    LogNotice("System parameter(GLOBAL_OVERRATE) value(%u/%u;%u).",
                m_GloabalOverrateRule.m_overRate_num_h,
                m_GloabalOverrateRule.m_overRate_time_h,
                m_GloabalOverrateRule.m_enable);

    return true;
}

bool ChannelSelect::ParseOverRateSysPara(STL_MAP_STR sysParamMap)
{
    STL_MAP_STR::iterator map = sysParamMap.find("OVERRATE");

    do
    {
        if(map == sysParamMap.end())
        {
            LogWarn("Can not find system parameter(OVERRATE).");
            break;
        }

        std::string strtmp = map->second;
        LogDebug("overRate sysparma is[%s]", strtmp.data());
        //strtmp[1/60;2/60;3/24;0]
        std::string::size_type pos = strtmp.find("/");
        if(std::string::npos != pos  && strtmp.length() > pos)
        {
            m_TempRule.m_overRate_num_s = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }

        pos = strtmp.find(";");
        if(std::string::npos != pos && strtmp.length() > pos)
        {
            m_TempRule.m_overRate_time_s = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }


        pos = strtmp.find("/");
        if(std::string::npos != pos && strtmp.length() > pos)
        {
            m_TempRule.m_overRate_num_m = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }

        pos = strtmp.find(";");
        if(std::string::npos != pos && strtmp.length() > pos)
        {
            m_TempRule.m_overRate_time_m = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }

        pos = strtmp.find("/");
        if(std::string::npos != pos && strtmp.length() > pos)
        {
            m_TempRule.m_overRate_num_h = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }

        pos = strtmp.find(";");
        if(std::string::npos != pos && strtmp.length() > pos)
        {
            m_TempRule.m_overRate_time_h = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }

        if(strtmp.length() == 1)
        {
            m_TempRule.m_enable = atoi(strtmp.data());
        }
    }
    while (0);

    LogNotice("System parameter(OVERRATE) value(%u/%u;%u/%u;%u/%u;%u).",
        m_TempRule.m_overRate_num_s,
        m_TempRule.m_overRate_time_s,
        m_TempRule.m_overRate_num_m,
        m_TempRule.m_overRate_time_m,
        m_TempRule.m_overRate_num_h,
        m_TempRule.m_overRate_time_h,
        m_TempRule.m_enable);

    return true;
}


bool ChannelSelect::ParseKeyWordOverRateSysPara(STL_MAP_STR sysParamMap)
{
    STL_MAP_STR::iterator map = sysParamMap.find("KEYWORD_OVERRATE");

    do
    {
        if(map == sysParamMap.end())
        {
            LogWarn("Can not find system parameter(KEYWORD_OVERRATE).");
            break;
        }

        std::string strtmp = map->second;
        LogDebug("key word overRate sysparma is[%s]", strtmp.data());
        //strtmp[1/60;2/60;3/24;0;0]
        //每秒限制（条/s）；每分钟限制（条/m）；每天限制（条/h）；超频开关（1为开启，0为关闭）;overrate_sign：0为（用户 + 签名），1为（* + 签名），2为（用户 + *）
        std::string::size_type pos = strtmp.find("/");
        if(std::string::npos != pos  && strtmp.length() > pos)
        {
            m_KeyWordTempRule.m_overRate_num_s = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }

        pos = strtmp.find(";");
        if(std::string::npos != pos && strtmp.length() > pos)
        {
            m_KeyWordTempRule.m_overRate_time_s = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }

        pos = strtmp.find("/");
        if(std::string::npos != pos && strtmp.length() > pos)
        {
            m_KeyWordTempRule.m_overRate_num_m = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }

        pos = strtmp.find(";");
        if(std::string::npos != pos && strtmp.length() > pos)
        {
            m_KeyWordTempRule.m_overRate_time_m = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }

        pos = strtmp.find("/");
        if(std::string::npos != pos && strtmp.length() > pos)
        {
            m_KeyWordTempRule.m_overRate_num_h = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }

        pos = strtmp.find(";");
        if(std::string::npos != pos && strtmp.length() > pos)
        {
            m_KeyWordTempRule.m_overRate_time_h = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }

        if(strtmp.length() == 3)
        {
            m_KeyWordTempRule.m_enable = atoi(strtmp.substr(0, 1).data());
            m_KeyWordTempRule.m_overrate_sign = atoi(strtmp.substr(2).data());
        }
        else if(strtmp.length() == 1)
        {
            m_KeyWordTempRule.m_enable = atoi(strtmp.data());
            m_KeyWordTempRule.m_overrate_sign = 1;
        }
    }
    while (0);


    LogNotice("System parameter(KEYWORD_OVERRATE) value(%u/%u;%u/%u;%u/%u;%u;%u).",
        m_KeyWordTempRule.m_overRate_num_s,
        m_KeyWordTempRule.m_overRate_time_s,
        m_KeyWordTempRule.m_overRate_num_m,
        m_KeyWordTempRule.m_overRate_time_m,
        m_KeyWordTempRule.m_overRate_num_h,
        m_KeyWordTempRule.m_overRate_time_h,
        m_KeyWordTempRule.m_enable,
        m_KeyWordTempRule.m_overrate_sign);

    return true;
}

bool ChannelSelect::ParseChannelOverRateSysPara(STL_MAP_STR sysParamMap)
{
    STL_MAP_STR::iterator map = sysParamMap.find("CHANNEL_OVERRATE");

    do
    {
        if(map == sysParamMap.end())
        {
            LogWarn("Can not find system parameter(CHANNEL_OVERRATE).");
            break;
        }

        std::string strtmp = map->second;
        LogDebug("CHANNEL_OVERRATE sysparam is[%s]", strtmp.data());
        //strtmp[1/60;2/60;3/24;0]
        std::string::size_type pos = strtmp.find("/");
        if(std::string::npos != pos  && strtmp.length() > pos)
        {
            m_ChannelSysOverrateRule.m_overRate_num_s = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }

        pos = strtmp.find(";");
        if(std::string::npos != pos && strtmp.length() > pos)
        {
            m_ChannelSysOverrateRule.m_overRate_time_s = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }


        pos = strtmp.find("/");
        if(std::string::npos != pos && strtmp.length() > pos)
        {
            m_ChannelSysOverrateRule.m_overRate_num_m = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }

        pos = strtmp.find(";");
        if(std::string::npos != pos && strtmp.length() > pos)
        {
            m_ChannelSysOverrateRule.m_overRate_time_m = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }

        pos = strtmp.find("/");
        if(std::string::npos != pos && strtmp.length() > pos)
        {
            m_ChannelSysOverrateRule.m_overRate_num_h = atoi(strtmp.substr(0, pos).data());
            strtmp = strtmp.substr(pos+1);
        }

        pos = strtmp.find(";");
        if(std::string::npos != pos && strtmp.length() > pos)
        {
            m_ChannelSysOverrateRule.m_overRate_time_h = atoi(strtmp.substr(0, pos).data()) * 24;
            strtmp = strtmp.substr(pos+1);
        }

        if(strtmp.length() == 1)
        {
            m_ChannelSysOverrateRule.m_enable = atoi(strtmp.data());
        }
    }
    while (0);

    LogNotice("System parameter(CHANNEL_OVERRATE) value(%u/%u;%u/%u;%u/%u;%u).",
        m_ChannelSysOverrateRule.m_overRate_num_s,
        m_ChannelSysOverrateRule.m_overRate_time_s,
        m_ChannelSysOverrateRule.m_overRate_num_m,
        m_ChannelSysOverrateRule.m_overRate_time_m,
        m_ChannelSysOverrateRule.m_overRate_num_h,
        m_ChannelSysOverrateRule.m_overRate_time_h,
        m_ChannelSysOverrateRule.m_enable);

    return true;
}


int ChannelSelect::getGlobalOverRateRule(models::TempRule &TempRule){
    LogDebug("Global overrate swich: %s, [%u/%u]", m_GloabalOverrateRule.m_enable == 0 ? "OFF" : "ON",
                                                    m_GloabalOverrateRule.m_overRate_num_h,
                                                    m_GloabalOverrateRule.m_overRate_time_h);

    TempRule = m_GloabalOverrateRule;

    return 0;
}

int ChannelSelect::getOverRateRule(std::string key, models::TempRule &TempRule)
{
    std::map<string ,models::TempRule>::iterator it = m_TempRuleMap.find(key);

    if(it != m_TempRuleMap.end() && it->second.m_state == 1)
    {
        //user has define. add system enable to it.
        TempRule = (*it).second;
        TempRule.m_enable = 1;

        LogDebug("use user level smstype overate rule, key: %s", key.c_str());
    }
    else
    {
        TempRule = m_TempRule;
        LogDebug("use sys level smstype overate rule, key: %s", key.c_str());
    }

    LogDebug("TempRule.m_enable[%u].", TempRule.m_enable);

    return 0;
}

void ChannelSelect::getChannelOverRateRuleAndKey(string strCId, TSmsRouterReq* pInfo, models::TempRule &TempRule, int& keyType)
{
    string strSmstype = pInfo->m_strSmsType;
    string& strSign = pInfo->m_strSign;
    string strMd5Sign = Comm::GetStrMd5(strSign);
    string star = "*";
    string strMd5Star = Comm::GetStrMd5(star);
    keyType = -1;

    do
    {
        if (m_ChannelOverrateMap.size() == 0)
        {
            TempRule = m_ChannelSysOverrateRule;
            if (m_ChannelSysOverrateRule.m_enable == 1)
            {
                keyType = CHANNEL_STAR_STAR;
                pInfo->m_strChannelOverRateKey = strCId + "_" + "*" + "_" + strMd5Star + "_" + pInfo->m_strPhone;
            }
            break;
        }

        string channelSmstypeSignKey = strCId + "_" + strSmstype + "_" + strSign;
        map<string,models::TempRule>::iterator channelSmstypeSign = m_ChannelOverrateMap.find(channelSmstypeSignKey);
        if(channelSmstypeSign != m_ChannelOverrateMap.end() && channelSmstypeSign->second.m_state == 1)
        {
            TempRule = (*channelSmstypeSign).second;
            TempRule.m_enable = 1;
            keyType = CHANNEL_SMSTYPE_SIGN;
            pInfo->m_strChannelOverRateKey = strCId + "_" + strSmstype + "_" + strMd5Sign + "_" + pInfo->m_strPhone;
            break;
        }

        string starSmstypeSignKey = string("*") + "_" + strSmstype + "_" + strSign;
        map<string,models::TempRule>::iterator starSmstypeSign = m_ChannelOverrateMap.find(starSmstypeSignKey);
        if(starSmstypeSign != m_ChannelOverrateMap.end() && starSmstypeSign->second.m_state == 1) //kai guan
        {
             TempRule = (*starSmstypeSign).second;
             TempRule.m_enable = 1;
             keyType = STAR_SMSTYPE_SIGN;
             pInfo->m_strChannelOverRateKey = strCId + "_" + strSmstype + "_" + strMd5Sign + "_" + pInfo->m_strPhone;
             break;
        }

        string channelStarSignKey = strCId + "_" + "*" + "_" + strSign;
        map<string,models::TempRule>::iterator channelStarSign = m_ChannelOverrateMap.find(channelStarSignKey);
        if(channelStarSign != m_ChannelOverrateMap.end() && channelStarSign->second.m_state == 1)
        {
             TempRule = (*channelStarSign).second;
             TempRule.m_enable = 1;
             keyType = CHANNEL_STAR_SIGN;
             pInfo->m_strChannelOverRateKey = strCId + "_" + "*" + "_" + strMd5Sign + "_" + pInfo->m_strPhone;
             break;
        }

        string channelSmstypeStarKey = strCId + "_" + strSmstype + "_" + "*";
        map<string,models::TempRule>::iterator channelSmstypeStar = m_ChannelOverrateMap.find(channelSmstypeStarKey);
        if(channelSmstypeStar != m_ChannelOverrateMap.end() && channelSmstypeStar->second.m_state == 1)
        {
             TempRule = (*channelSmstypeStar).second;
             TempRule.m_enable = 1;
             keyType = CHANNEL_SMSTYPE_STAR;
             pInfo->m_strChannelOverRateKey = strCId + "_" + strSmstype + "_" + strMd5Star + "_" + pInfo->m_strPhone;
             break;
        }

        string channelStarStarKey = strCId + "_" + "*" + "_" + "*";
        map<string,models::TempRule>::iterator channelStarStar = m_ChannelOverrateMap.find(channelStarStarKey);
        if(channelStarStar != m_ChannelOverrateMap.end() && channelStarStar->second.m_state == 1)
        {
             TempRule = (*channelStarStar).second;
             TempRule.m_enable = 1;
             keyType = CHANNEL_STAR_STAR;
             pInfo->m_strChannelOverRateKey = strCId + "_" + "*" + "_" + strMd5Star + "_" + pInfo->m_strPhone;
             break;
        }

        string star = "*";
        string starStarSignKey = star + "_" + "*" + "_" + strSign;
        map<string,models::TempRule>::iterator starStarSign = m_ChannelOverrateMap.find(starStarSignKey);
        if(starStarSign != m_ChannelOverrateMap.end() && starStarSign->second.m_state == 1)
        {
             TempRule = (*starStarSign).second;
             TempRule.m_enable = 1;
             keyType = STAR_STAR_SIGN;
             pInfo->m_strChannelOverRateKey = strCId + "_" + "*" + "_" + strMd5Sign + "_" + pInfo->m_strPhone;
             break;
        }

        string starSmstypeStarKey = star + "_" + strSmstype + "_" + "*";
        map<string,models::TempRule>::iterator starSmstypeStar = m_ChannelOverrateMap.find(starSmstypeStarKey);
        if(starSmstypeStar != m_ChannelOverrateMap.end() && starSmstypeStar->second.m_state == 1)
        {
             TempRule = (*starSmstypeStar).second;
             TempRule.m_enable = 1;
             keyType = STAR_SMSTYPE_STAR;
             pInfo->m_strChannelOverRateKey = strCId + "_" + strSmstype + "_" + strMd5Star + "_" + pInfo->m_strPhone;
             break;
        }

        TempRule = m_ChannelSysOverrateRule;//系统参数表
        if (m_ChannelSysOverrateRule.m_enable == 1)
        {
            keyType = CHANNEL_STAR_STAR;
            pInfo->m_strChannelOverRateKey = strCId + "_" + "*" + "_" + strMd5Sign + "_" + pInfo->m_strPhone;
        }
    }
    while (0);

    LogNotice("channel rule:cid = %s, strSmstype = %s, sign = %s, smsid = %s, phone = %s, enable = %u, ChannelOverRateKey = %s "
        ", m_overRate_num_s[%d/%d],m_overRate_num_m[%d/%d], m_overRate_num_d[%d/%d]"
        , strCId.data(), strSmstype.data(), strSign.data(), pInfo->m_strSmsId.data(), pInfo->m_strPhone.data()
        , TempRule.m_enable, pInfo->m_strChannelOverRateKey.data(), TempRule.m_overRate_num_s, TempRule.m_overRate_time_s
        , TempRule.m_overRate_num_m, TempRule.m_overRate_time_m, TempRule.m_overRate_num_h, TempRule.m_overRate_time_h/24);


}

void ChannelSelect::getChannelOverRateRuleAndKey(cs_t strChannelId,
                                                cs_t strSmstype,
                                                cs_t strSign,
                                                cs_t strPhone,
                                                string& strChannelOverRateKey,
                                                models::TempRule &TempRule,
                                                int& keyType)
{
    string strMd5Sign = Comm::GetStrMd5(strSign);
    string star = "*";
    string strMd5Star = Comm::GetStrMd5(star);
    keyType = -1;

    do
    {
        if (m_ChannelOverrateMap.size() == 0)
        {
            TempRule = m_ChannelSysOverrateRule;
            if (m_ChannelSysOverrateRule.m_enable == 1)
            {
                keyType = CHANNEL_STAR_STAR;
                strChannelOverRateKey = strChannelId + "_" + "*" + "_" + strMd5Star + "_" + strPhone;
            }
            break;
        }

        string channelSmstypeSignKey = strChannelId + "_" + strSmstype + "_" + strSign;
        map<string,models::TempRule>::iterator channelSmstypeSign = m_ChannelOverrateMap.find(channelSmstypeSignKey);
        if(channelSmstypeSign != m_ChannelOverrateMap.end() && channelSmstypeSign->second.m_state == 1)
        {
            TempRule = (*channelSmstypeSign).second;
            TempRule.m_enable = 1;
            keyType = CHANNEL_SMSTYPE_SIGN;
            strChannelOverRateKey = strChannelId + "_" + strSmstype + "_" + strMd5Sign + "_" + strPhone;
            break;
        }

        string starSmstypeSignKey = string("*") + "_" + strSmstype + "_" + strSign;
        map<string,models::TempRule>::iterator starSmstypeSign = m_ChannelOverrateMap.find(starSmstypeSignKey);
        if(starSmstypeSign != m_ChannelOverrateMap.end() && starSmstypeSign->second.m_state == 1) //kai guan
        {
             TempRule = (*starSmstypeSign).second;
             TempRule.m_enable = 1;
             keyType = STAR_SMSTYPE_SIGN;
             strChannelOverRateKey = strChannelId + "_" + strSmstype + "_" + strMd5Sign + "_" + strPhone;
             break;
        }

        string channelStarSignKey = strChannelId + "_" + "*" + "_" + strSign;
        map<string,models::TempRule>::iterator channelStarSign = m_ChannelOverrateMap.find(channelStarSignKey);
        if(channelStarSign != m_ChannelOverrateMap.end() && channelStarSign->second.m_state == 1)
        {
             TempRule = (*channelStarSign).second;
             TempRule.m_enable = 1;
             keyType = CHANNEL_STAR_SIGN;
             strChannelOverRateKey = strChannelId + "_" + "*" + "_" + strMd5Sign + "_" + strPhone;
             break;
        }

        string channelSmstypeStarKey = strChannelId + "_" + strSmstype + "_" + "*";
        map<string,models::TempRule>::iterator channelSmstypeStar = m_ChannelOverrateMap.find(channelSmstypeStarKey);
        if(channelSmstypeStar != m_ChannelOverrateMap.end() && channelSmstypeStar->second.m_state == 1)
        {
             TempRule = (*channelSmstypeStar).second;
             TempRule.m_enable = 1;
             keyType = CHANNEL_SMSTYPE_STAR;
             strChannelOverRateKey = strChannelId + "_" + strSmstype + "_" + strMd5Star + "_" + strPhone;
             break;
        }

        string channelStarStarKey = strChannelId + "_" + "*" + "_" + "*";
        map<string,models::TempRule>::iterator channelStarStar = m_ChannelOverrateMap.find(channelStarStarKey);
        if(channelStarStar != m_ChannelOverrateMap.end() && channelStarStar->second.m_state == 1)
        {
             TempRule = (*channelStarStar).second;
             TempRule.m_enable = 1;
             keyType = CHANNEL_STAR_STAR;
             strChannelOverRateKey = strChannelId + "_" + "*" + "_" + strMd5Star + "_" + strPhone;
             break;
        }

        string star = "*";
        string starStarSignKey = star + "_" + "*" + "_" + strSign;
        map<string,models::TempRule>::iterator starStarSign = m_ChannelOverrateMap.find(starStarSignKey);
        if(starStarSign != m_ChannelOverrateMap.end() && starStarSign->second.m_state == 1)
        {
             TempRule = (*starStarSign).second;
             TempRule.m_enable = 1;
             keyType = STAR_STAR_SIGN;
             strChannelOverRateKey = strChannelId + "_" + "*" + "_" + strMd5Sign + "_" + strPhone;
             break;
        }

        string starSmstypeStarKey = star + "_" + strSmstype + "_" + "*";
        map<string,models::TempRule>::iterator starSmstypeStar = m_ChannelOverrateMap.find(starSmstypeStarKey);
        if(starSmstypeStar != m_ChannelOverrateMap.end() && starSmstypeStar->second.m_state == 1)
        {
             TempRule = (*starSmstypeStar).second;
             TempRule.m_enable = 1;
             keyType = STAR_SMSTYPE_STAR;
             strChannelOverRateKey = strChannelId + "_" + strSmstype + "_" + strMd5Star + "_" + strPhone;
             break;
        }

        TempRule = m_ChannelSysOverrateRule;//系统参数表
        if (m_ChannelSysOverrateRule.m_enable == 1)
        {
            keyType = CHANNEL_STAR_STAR;
            strChannelOverRateKey = strChannelId + "_" + "*" + "_" + strMd5Sign + "_" + strPhone;
        }
    }
    while (0);

    LogDebug("channel overrate rule ==> cid[%s],strSmstype[%s],sign[%s],phone[%s],enable[%u],ChannelOverRateKey[%s], "
        "m_overRate_num_s[%d/%d],m_overRate_num_m[%d/%d],m_overRate_num_d[%d/%d]",
        strChannelId.data(),
        strSmstype.data(),
        strSign.data(),
        strPhone.data(),
        TempRule.m_enable,
        strChannelOverRateKey.data(),
        TempRule.m_overRate_num_s,
        TempRule.m_overRate_time_s,
        TempRule.m_overRate_num_m,
        TempRule.m_overRate_time_m,
        TempRule.m_overRate_num_h,
        TempRule.m_overRate_time_h/24);
}


int ChannelSelect::getKeyWordOverRateRule(string& strClientId, string strSign, models::TempRule &TempRule, int& keyType)
{
    string userSignKey = strClientId + "_" + strSign;
    map<string,models::TempRule>::iterator userSign = m_KeyWordTempRuleMap.find(userSignKey);
    if(userSign != m_KeyWordTempRuleMap.end() && userSign->second.m_state == 1)
    {
        TempRule = (*userSign).second;
        TempRule.m_enable = 1;
        keyType = OVERRATE_KEYWORD_RULE_USER_SIGN;
    }
    else
    {
        string starSignKey = string("*") + "_" + strSign;
        map<string,models::TempRule>::iterator starSign = m_KeyWordTempRuleMap.find(starSignKey);
        if(starSign != m_KeyWordTempRuleMap.end() && starSign->second.m_state == 1) //kai guan
        {
             TempRule = (*starSign).second;
             TempRule.m_enable = 1;
             keyType = OVERRATE_KEYWORD_RULE_STAR_SIGN;
        }
        else
        {
            string userStarKey = strClientId + "_" + "*";
            map<string,models::TempRule>::iterator userStar = m_KeyWordTempRuleMap.find(userStarKey);
            if(userStar != m_KeyWordTempRuleMap.end() && userStar->second.m_state == 1)
            {
                 TempRule = (*userStar).second;
                 TempRule.m_enable = 1;
                 keyType = OVERRATE_KEYWORD_RULE_USER_STAR;
            }
            else
            {
                TempRule = m_KeyWordTempRule;//系统参数表
                if (m_KeyWordTempRule.m_enable == 1)
                {
                    if (m_KeyWordTempRule.m_overrate_sign == 0)
                    {
                        keyType = OVERRATE_KEYWORD_RULE_USER_SIGN;
                    }
                    else if (m_KeyWordTempRule.m_overrate_sign == 1)
                    {
                        keyType = OVERRATE_KEYWORD_RULE_STAR_SIGN;
                    }
                    else if (m_KeyWordTempRule.m_overrate_sign == 2)
                    {
                        keyType = OVERRATE_KEYWORD_RULE_USER_STAR;
                    }
                    else
                    {
                        keyType = -1;
                        LogWarn("can not find overrate key word rule in system param,overrate_sign error clientid = %s, sign = %s, overrate_sign = %u"
                            , strClientId.data(), strSign.data(), m_KeyWordTempRule.m_overrate_sign);
                    }
                }
                else
                {
                    keyType = -1;
                    LogWarn("can not find overrate key word rule, clientid = %s, sign = %s", strClientId.data(), strSign.data());
                }
            }
        }

    }

    LogNotice("use rule:user = %s, sign = %s, enable =  %u, m_overrate_sign = %u, m_strUserID = %s, m_strSign = %s",
        strClientId.data(),
        strSign.data(),
        TempRule.m_enable,
        TempRule.m_overrate_sign,
        TempRule.m_strUserID.data(),
        TempRule.m_strSign.data());

    return 0;
}


int ChannelSelect::getPhoneArea(string strPhone)
{
    std::string phonehead7 = strPhone.substr(0,7);
    std::string phonehead8 = strPhone.substr(0,8);
    int ret = 0;

    //LogDebug("m_PhoneAreaMap.size[%d]", m_PhoneAreaMap.size());
    do
    {
        std::map<std::string, PhoneSection>::iterator it = m_PhoneAreaMap.find(phonehead7); //key: phone head 7/8
        if(it != m_PhoneAreaMap.end())
        {
            ret =  it->second.area_id;
            break;
        }

        it = m_PhoneAreaMap.find(phonehead8);
        if(it != m_PhoneAreaMap.end())
        {
            ret =  it->second.area_id;
            break;
        }
    }while(0);

    return ret;
}

int ChannelSelect::getPhoneCode(string strPhone)
{
    std::string phonehead7 = strPhone.substr(0,7);
    std::string phonehead8 = strPhone.substr(0,8);
    int ret = 0;

    //LogDebug("m_PhoneAreaMap.size[%d]", m_PhoneAreaMap.size());
    do
    {
        std::map<std::string, PhoneSection>::iterator it = m_PhoneAreaMap.find(phonehead7); //key: phone head 7/8
        if(it != m_PhoneAreaMap.end())
        {
            ret =  it->second.code;
            break;
        }

        it = m_PhoneAreaMap.find(phonehead8);
        if(it != m_PhoneAreaMap.end())
        {
            ret =  it->second.code;
            break;
        }
    }while(0);

    return ret;
}

bool ChannelSelect::getSmppPrice(std::string phone, UInt32 channelId,int& salePrice,int& costPrice)
{
    std::string prefix;
    for(int i= PREFIXLEN; i>2; i--)
    {
        prefix= phone.substr(2,i-2);
        char chtmp[100] = {0};
        snprintf(chtmp, 100,"%d&%s", channelId, prefix.data());

        STL_MAP_STR::iterator itrCost = m_PriceMap.find(string(chtmp));     //key = channelid + "&" + prefix;

        STL_MAP_STR::iterator itrSale = m_SalePriceMap.find(prefix);

        if((itrCost != m_PriceMap.end()) && (itrSale != m_SalePriceMap.end()))
        {
            ////LogDebug("==debug== costFee[%s],saleFee[%s].",itrCost->second.data(),itrSale->second.data());
            costPrice = atoi(itrCost->second.data());
            salePrice = atoi(itrSale->second.data());
            return true;
        }
    }

    LogWarn("warning!!! cat't find smpp price. phone[%s], channelID[%d]", phone.data(), channelId);
    return false;
}

bool ChannelSelect::getSmppSalePrice(std::string phone,int& salePrice)
{
    std::string prefix;
    for(int i= PREFIXLEN; i>2; i--)
    {
        prefix= phone.substr(2,i-2);
        STL_MAP_STR::iterator itrSale = m_SalePriceMap.find(prefix);
        if(itrSale != m_SalePriceMap.end())
        {
            ////LogDebug("==debug== saleFee[%s].",itrSale->second.data());
            salePrice = atoi(itrSale->second.data());
            return true;
        }
    }

    LogWarn("warning!!! cat't find smpp Sale price. phone[%s]", phone.data());
    return false;
}


bool  ChannelSelect::getExtendPort(UInt32 channelId, string& strSign,string& strClientId,string& strExtPort)
{
    char chtmp[250] = {0};
    snprintf(chtmp, 250,"%s&%u&%s", strSign.data(), channelId,strClientId.data());
    std::map<std::string, SignExtnoGw>::iterator it = m_SignextnoGwMap.find(string(chtmp));
    if(it != m_SignextnoGwMap.end())
    {
        strExtPort = it->second.appendid;
        return true;
    }
    else
    {
        snprintf(chtmp, 250,"%s&%u&*", strSign.data(), channelId);
        std::map<std::string, SignExtnoGw>::iterator it = m_SignextnoGwMap.find(string(chtmp));
        if(it != m_SignextnoGwMap.end())
        {
            strExtPort = it->second.appendid;
            return true;
        }

        return false;
    }
}

bool ChannelSelect::getChannelExtendPort(UInt32 channelId,string& strClientId,string& strExtPort)
{
    char cTemp[250] = {0};
    snprintf(cTemp,250,"%u&%s",channelId,strClientId.data());

    STL_MAP_STR::iterator iter  = m_ChannelExtendPortTable.find(string(cTemp));
    if (iter != m_ChannelExtendPortTable.end())
    {
        strExtPort = iter->second;
        return true;
    }
    else
    {
        return false;
    }
}

bool ChannelSelect::getChannelGroupVec(string strCliendID, string strSid, string strSmsType, UInt32 uSmsFrom, UInt32 uOperator, std::vector<std::string>& vecChannelGroups,models::UserGw& userGw)
{
    if (uSmsFrom > 1 )   //access
    {
        getUserGw(strCliendID + "_" + strSmsType, userGw);      //clientID_type
    }
    else    //rest vmsp
    {
        getUserGw(strSid + "_" + strSmsType, userGw);       //sid_type
    }

    string strChannelGroups;

    if (userGw.userid.empty())
    {
        LogWarn("getChannelGroupVec failed,can not find userGw."
            "clientid[%s],sid[%s],smstype[%s],smsfrom[%d].",
            strCliendID.data(),
            strSid.data(),
            strSmsType.data(),
            uSmsFrom);

        return false;
    }
    else
    {
        LogDebug("get channelGroups OK, uOperator[%d],distoperators[%d],qw_GROUP[%s],"
            "yd_GROUP[%s],lt_GROUP[%s],dx_GROUP[%s],gx_GROUP[%s],xnyd_GROUP[%s],xnlt_GROUP[%s],xndx_GROUP[%s].",
            uOperator,
            userGw.distoperators,
            userGw.m_strChannelID.data(),
            userGw.m_strYDChannelID.data(),
            userGw.m_strLTChannelID.data(),
            userGw.m_strDXChannelID.data(),
            userGw.m_strGJChannelID.data(),
            userGw.m_strXNYDChannelID.data(),
            userGw.m_strXNLTChannelID.data(),
            userGw.m_strXNDXChannelID.data());

        if (uOperator == FOREIGN)
        {
            strChannelGroups = userGw.m_strGJChannelID;
        }
        else if (uOperator == XNYD)
        {
            strChannelGroups = userGw.m_strXNYDChannelID;
        }
        else if (uOperator == XNLT)
        {
            strChannelGroups = userGw.m_strXNLTChannelID;
        }
        else if (uOperator == XNDX)
        {
            strChannelGroups = userGw.m_strXNDXChannelID;
        }
        else if (userGw.distoperators)
        {
            switch(uOperator)
            {
                case YIDONG:
                {
                    strChannelGroups = userGw.m_strYDChannelID;
                    break;
                }
                case LIANTONG:
                {
                    strChannelGroups = userGw.m_strLTChannelID;
                    break;
                }
                case DIANXIN:
                {
                    strChannelGroups = userGw.m_strDXChannelID;
                    break;
                }
                default:
                {
                    LogError("cat not get channleGroup. no reason to come here, uOperator[%d]", uOperator);
                    return false;
                }
            }
        }
        else
        {
            strChannelGroups = userGw.m_strChannelID;       //QUANWANG
        }
    }

    Comm::splitExVector(strChannelGroups, ",", vecChannelGroups);

    if (strChannelGroups.empty() || vecChannelGroups.size() == 0)
    {
        LogWarn("strChannelGroups is null, or vecChannels.size==0");
        return false;
    }

    return true;
}





