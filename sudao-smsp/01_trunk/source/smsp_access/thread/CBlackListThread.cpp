/*************************************************************************
copyright (C),2018-2030,ucpaas .Co.,Ltd.

FileName     : CBlackListThread.cpp
Author       : pengzhen      Version : 1.0    Date: 2018/07/18
Description  : 黑名单线程处理类
Version      : 1.0
Function List :
**************************************************************************/


#include "UrlCode.h"
#include "UTFString.h"
#include "Comm.h"
#include "HttpParams.h"
#include "base64.h"
#include "CotentFilter.h"
#include "CBlackListThread.h"
#include "CRedisThread.h"
#include "LogMacro.h"
#include "ErrorCode.h"
#include "main.h"

using namespace models;

CBlackListThread::CBlackListThread(const char *name)
        : CThread(name)
{
    m_bBlankPhoneSwitch = false;
}

CBlackListThread::~CBlackListThread()
{
}

bool CBlackListThread::Init()
{
    STL_MAP_STR mapSysPara;
    g_pRuleLoadThread->getSysParamMap( mapSysPara );
    GetSysPara(mapSysPara);
    g_pRuleLoadThread->getUserGwMap( m_userGwMap );
    g_pRuleLoadThread->getBlacklistGrp( m_uBlacklistGrpMap );
    g_pRuleLoadThread->getBlacklistCatAll( m_uBgroupRefCategoryMap );
    return true;
}

void CBlackListThread::GetSysPara( const STL_MAP_STR& mapSysPara )
{
    string strSysPara = "";
    string strBlankPhoneErr = "";
    std::map<std::string, std::string>::const_iterator iter;

    do
    {
        strSysPara = "BLANK_PHONE_CFG";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        std::string strTmp = iter->second;
        if (strTmp.empty())
        {
            LogError("system parameter(%s) is empty, pls reconfig.", strSysPara.c_str());
            break;
        }

        int index = -1;
        if(-1 == (index = strTmp.find(";")))
        {
            LogError("system parameter(%s) cfg(%s)format error",strSysPara.c_str(),strTmp.c_str());
            break;
        }

        vector<std::string> vecCfg;
        Comm::split(strTmp, ";", vecCfg);
        if (vecCfg.size() < 3)
        {
            LogError("system parameter(%s) cfg(%s)format error",strSysPara.c_str(),strTmp.c_str());
        }
        else
        {
            m_bBlankPhoneSwitch = (1 == atoi(vecCfg[1].c_str()));
            if (vecCfg[2].length() >= 8)
                strBlankPhoneErr = vecCfg[2];
        }

    }
    while (0);

    LogNotice("System parameter(%s) value  BlankPhoneSwitch[ %s ], blankPhoneErr[ %s ]",
              strSysPara.c_str(), m_bBlankPhoneSwitch ? "ON" : "OFF",
              strBlankPhoneErr.c_str());
}


void CBlackListThread::HandleMsg( TMsg* pMsg )
{
    switch ( pMsg->m_iMsgType )
    {
        case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
        {
            TUpdateSysParamRuleReq* pSysParam = ( TUpdateSysParamRuleReq* )pMsg;
            GetSysPara( pSysParam->m_SysParamMap );
            LogNotice("update sys param config ");
            break;
        }
        case MSGTYPE_RULELOAD_BLGRP_REF_CATEGORY_UPDATE_REQ:
        {
            TUpdateBlGrpRefCatConfigReq* pBLGrpRefCat = ( TUpdateBlGrpRefCatConfigReq* )pMsg;
            m_uBgroupRefCategoryMap = pBLGrpRefCat->m_uBgroupRefCategoryMap;       
            LogNotice("update t_sms_blacklist_group_ref_category %u", m_uBgroupRefCategoryMap.size());
            break;
        }
        case MSGTYPE_RULELOAD_BLUSER_CONFIG_GROUP_UPDATE_REQ:
        {
            TUpdateBlUserConfigGrpReq* pUserConfigBLGrp = ( TUpdateBlUserConfigGrpReq* )pMsg;
            m_uBlacklistGrpMap = pUserConfigBLGrp->m_uBlacklistGrpMap;
            LogNotice("t_sms_blacklist_user_config %u", m_uBlacklistGrpMap.size());
            break;
        }
        case MSGTYPE_RULELOAD_USERGW_UPDATE_REQ:
        {
            TUpdateUserGwReq *pReq = (TUpdateUserGwReq *)pMsg;
            if (pReq)
            {
                m_userGwMap.clear();
                m_userGwMap = pReq->m_UserGwMap;
                LogNotice("RuleUpdate userGwMap update. map.size[%d]", pReq->m_UserGwMap.size());
            }
            break;
        }
        default:
            ;;
    }
    return ;
}

/*include sysblacklist and channel blacklist*/
UInt32 CBlackListThread::GetRedisBlackListInfo( session_pt pSession )
{
    //key:[black_list:Phonenum]
    string strBlackListKey = "black_list:" ;
    strBlackListKey.append( pSession->m_strPhone );

    string strRedisCmd = "GET " ;
    strRedisCmd.append( strBlackListKey );

    redisGet( strRedisCmd, strBlackListKey, g_pSessionThread, pSession->m_uSessionId, "check blacklist" );

    return ACCESS_ERROR_NONE ;
}

bool CBlackListThread::ProUserBlackList(session_pt pSession)
{
    int userSmstype = 0;
    set<string>::iterator itr = pSession->m_vecPhoneBlackListInfo.begin();
    for(; itr != pSession->m_vecPhoneBlackListInfo.end(); itr++)
    {
        string key = pSession->m_strClientId + ":";
        if(string::npos != (*itr).find(key))
        {
            userSmstype |= atoi((*itr).substr(pSession->m_strClientId.length() + 1).data());
        }
    }

    if( userSmstype & Blacklist::GetSmsTypeFlag(pSession->m_strSmsType))
    {
        pSession->m_strErrorCode = USER_BLACKLIST;
        pSession->m_strYZXErrCode = USER_BLACKLIST;
        return false;
    }

    return true;

}

bool CBlackListThread::CheckBlacklistClassify(session_pt pSession, UInt32 uBLGrp)
{
    UInt64 uCatgory = 0;
    map<UInt32,UInt64>::iterator itCat = m_uBgroupRefCategoryMap.find(uBLGrp);
    if (itCat == m_uBgroupRefCategoryMap.end())
    {
         LogError("[%s:%s:%s] smsType[%s] can not find blacklist catgory in group[%u]",
             pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str(),
             pSession->m_strSmsType.c_str(), uBLGrp);
         return true;
    }
    else
    {
        set<string>::iterator itr = pSession->m_vecPhoneBlackListInfo.begin();
        for(; itr != pSession->m_vecPhoneBlackListInfo.end(); itr++)
        {
            string key("c:");
            if(string::npos != (*itr).find(key))
            {
                uCatgory |= atoi((*itr).substr(key.length()).data());
            }
        }
        if (itCat->second & uCatgory)
        {
            LogElk("[%s:%s:%s] smsType[%s],[%lu:%lu]",
                pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str(),
                pSession->m_strSmsType.c_str(), itCat->second, uCatgory);
            return false;
        }
        else
            return true;
    }
    
}

bool CBlackListThread::ProBlacklistClassify(session_pt pSession)
{
    string strKey = pSession->m_strClientId + "_" +  Comm::int2str(Blacklist::GetSmsTypeFlag(pSession->m_strSmsType));
    map<string,UInt32>::iterator itrGrp =  m_uBlacklistGrpMap.find(strKey);
    if (itrGrp == m_uBlacklistGrpMap.end())
    {
         string strKeyDef = string("*") + "_" + Comm::int2str(Blacklist::GetSmsTypeFlag(pSession->m_strSmsType));
         map<string,UInt32>::iterator itrGrpDef =  m_uBlacklistGrpMap.find(strKeyDef);
         if (itrGrpDef == m_uBlacklistGrpMap.end())
         {
             LogError("[%s:%s:%s] smsType[%s] can not find blacklist group",
                pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str(),
                pSession->m_strSmsType.c_str());
             return true;
         }
         else
         {
             return CheckBlacklistClassify(pSession, itrGrpDef->second);
         }
    }
    else
    {
        return CheckBlacklistClassify(pSession, itrGrp->second);    
    }
}

bool CBlackListThread::CheckSysBlackList( session_pt pSession )
{
    if( !pSession )
    {
        LogError("pSession is null!!");
        return true;
    }
    
    if (ProUserBlackList( pSession ))
    {
        std::string strKey = pSession->m_strClientId + "_" + pSession->m_strSmsType;
        USER_GW_MAP::iterator itrGW = m_userGwMap.find(strKey);
        if (itrGW == m_userGwMap.end() || 1 == itrGW->second.m_uUnblacklist)
        {
            LogWarn("[%s:%s:%s] smsType[%s] is not find in table userGw or free",
                pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str(),
                pSession->m_strSmsType.c_str());
            return true;
        }
        else
        {
            return ProBlacklistClassify(pSession);            
        }        
    }
    else
    {
        return false;
    }

    return true;

}

bool CBlackListThread::CheckBlacklistRedisResp( redisReply* pRedisReply, session_pt pSession )
{
    CHK_NULL_RETURN_FALSE(pSession);
    
    std::string strOut = "";

    pSession->m_vecPhoneBlackListInfo.clear();

    if( NULL == pRedisReply )
    {
        LogWarn("[%s:%s:%s] redis reply is NULL.",
                pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str());
    }
    else if ( REDIS_REPLY_STRING == pRedisReply->type )
    {
        strOut = pRedisReply->str;
        LogNotice("[%s:%s:%s] redis blacklistinfo: %s",
                pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str(),
                strOut.c_str());

        if(!strOut.empty())
        {
            /*保存黑名单数据，用于通道路由时过滤通道黑名单*/
            Comm::split( strOut, "&", pSession->m_vecPhoneBlackListInfo );
            if (pSession->m_uFromSmsReback)
            {
                LogDebug("not need to check sys and user blacklist and blankphone,from reback");
                return false;
            }
            if ( false == CheckSysBlackList( pSession ))
            {
                if( pSession->m_strErrorCode.empty())
                {
                    pSession->m_strErrorCode = BLACKLIST;
                    pSession->m_strYZXErrCode = BLACKLIST;
                }
                LogWarn("[%s:%s:%s] matched in sys blackList[%s].",
                        pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str(),
                        strOut.c_str());
                return true ;
            }
            else
            {
                if ( m_bBlankPhoneSwitch)
                {
                    set<string>::iterator itr = pSession->m_vecPhoneBlackListInfo.find("blank");
                    if ( itr != pSession->m_vecPhoneBlackListInfo.end() )
                    {
                        LogWarn("[%s:%s:%s] phone is blank", 
                            pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str());
                        pSession->m_strErrorCode = BLANK_PHONE_ERROR;
                        pSession->m_strYZXErrCode = BLANK_PHONE_ERROR;
                        pSession->m_strExtraErrorDesc = BLANK_PHONE_ERROR;
                        return true;
                    }
                }
            }
        }

    }

    return false;

}

//what to do with timeout, alarm or record it??
UInt32 CBlackListThread::GetBlackTimeOut(TMsg* pMsg, session_pt pSession )
{
    return ACCESS_ERROR_NONE;
}










