#include "CommClass.h"
#include "LogMacro.h"
#include "BusTypes.h"
#include <sys/time.h>

#include "CSessionThread.h"
#include "CTemplateThread.h"
#include "CAuditThread.h"
#include "CRouterThread.h"
#include "COverRateThread.h"

using namespace BusType;
using namespace std;


SmsAccount* GetAccountInfo( string &clientid, accountMap &mapAccount )
{
    accountMap::iterator itr = mapAccount.find( clientid );
    if ( itr != mapAccount.end())
    {
        return &itr->second;
    }

    return NULL;

}

MqConfig* GetMQConfig( UInt32 mqId, mqConfigMap &conifgMQMap )
{
    mqConfigMap::iterator itr = conifgMQMap.find( mqId );
    if( itr != conifgMQMap.end() )
    {
        return &itr->second;
    }
    return NULL;
}


MqConfig* GetMQConfig( string &strKey, componentRefMQMap &refMQMap, mqConfigMap &conifgMQMap )
{
    componentRefMQMap::iterator itReq =  refMQMap.find( strKey );
    if ( itReq == refMQMap.end() )
    {
        LogError("==except== strKey:%s is not find in m_componentRefMqInfo.",strKey.data());
        return NULL;
    }

    mqConfigMap::iterator itrMq = conifgMQMap.find( itReq->second.m_uMqId );
    if ( itrMq == conifgMQMap.end())
    {
        LogError("==except== mqid:%lu is not find in m_mqConfigInfo.",itReq->second.m_uMqId);
        return NULL ;
    }

    if( itrMq->second.m_strExchange.empty()
        || itrMq->second.m_strRoutingKey.empty() )
    {
        LogError("mqid[%ld], m_strExchange[%s], m_strRoutingKey[%s]",
                 itrMq->second.m_uMqId, itrMq->second.m_strExchange.c_str(),
                 itrMq->second.m_strRoutingKey.c_str());
        return NULL;
    }

    return &itrMq->second;

}


ComponentConfig* getComponentConfig( string &strKeyId, compoentMap &compMap )
{
    compoentMap::iterator itr = compMap.find( strKeyId );
    if( itr == compMap.end() )
    {
        LogError("find compent %s failed", strKeyId.c_str());
        return NULL;
    }
    return &itr->second;
}

bool GetAgentInfo( UInt64 AgentId, AgentInfo & agent )
{
    return true;
}


UInt32 GetSendType( string& strSmsType )
{
    int iSmsType = atoi(strSmsType.c_str());
    UInt32 sendtype ;
    if(SMS_TYPE_NOTICE == iSmsType ||
       SMS_TYPE_VERIFICATION_CODE == iSmsType ||
       SMS_TYPE_ALARM == iSmsType)
    {
        sendtype = SEND_TYPE_HY;
    }
    else
    {
        sendtype = SEND_TYPE_YX;
    }
    return sendtype;
}

UInt32 GetSmsTypeFlag(string& strSmsType)
{
    int iSmsType = atoi(strSmsType.c_str());
    switch(iSmsType)
    {
        case SMS_TYPE_NOTICE:
            return SMS_TYPE_NOTICE_FLAG;

        case SMS_TYPE_VERIFICATION_CODE:
            return SMS_TYPE_VERIFICATION_CODE_FLAG;

        case SMS_TYPE_MARKETING:
            return SMS_TYPE_MARKETING_FLAG;

        case SMS_TYPE_ALARM:
            return SMS_TYPE_ALARM_FLAG;

        case SMS_TYPE_USSD:
            return SMS_TYPE_USSD_FLAG;

        case SMS_TYPE_FLUSH_SMS:
            return SMS_TYPE_FLUSH_SMS_FLAG;

        default:
            return 0;
    }
    return 0;
}

UInt32 GetPhoneArea( cs_t strPhone, phoneAreaMap& areaPhoneMap )
{
    std::string phonehead7 = strPhone.substr(0,7);
    std::string phonehead8 = strPhone.substr(0,8);
    int ret = 0;
    do
    {
        phoneAreaMap::iterator it = areaPhoneMap.find(phonehead7); //key: phone head 7/8
        if(it != areaPhoneMap.end())
        {
            ret = it->second.area_id;
            break;
        }

        it = areaPhoneMap.find ( phonehead8 );
        if(it != areaPhoneMap.end())
        {
            ret =  it->second.area_id;
            break;
        }

    }while(0);

    return ret;

}


#define CASE( state ) case state: return #state
string state2String( UInt32 uState )
{
    switch ( uState )
    {
        CASE( STATE_INIT );
        CASE( STATE_BLACKLIST );
        CASE( STATE_TEMPLATE  );
        CASE( STATE_AUDIT     );
        CASE( STATE_ROUTE     );
        CASE( STATE_OVERRATE  );
        CASE( STATE_CHARGE    );
        CASE( STATE_DONE      );
        default:
            return "UNKOWN ";
    }
    return "UNKOWN";
}

string error2Str( UInt32 uError )
{
    switch( uError )
    {
        CASE(ACCESS_ERROR_NONE);
        CASE(ACCESS_ERROR_PARAM_NULL);
        CASE(ACCESS_ERROR_COMM_POST);
        CASE(ACCESS_ERROR_ACCOUNT_NOT_EXSIT);
        CASE(ACCESS_ERROR_AGENT_ERROR);
        CASE(ACCESS_ERROR_SESSION_NOT_EXSIT);
        CASE(ACCESS_ERROR_SESSION_STAT_WRONG);
        CASE(ACCESS_ERROR_SESSION_TIMEOUT);
        CASE(ACCESS_ERROR_PHONE_BLACK);
        CASE(ACCESS_ERROR_PHONE_FORMAT);
        CASE(ACCESS_ERROR_MQ_NOT_FIND);
        CASE(ACCESS_ERROR_MQ_ROUTER_EMPTY);
        CASE(ACCESS_ERROR_CHARGE);
        CASE(ACCESS_ERROR_CHARGE_POST);
        CASE(ACCESS_ERROR_CHARGE_URL);
        CASE(ACCESS_ERROR_CHARGE_SENDER);
        CASE(ACCESS_ERROR_DISPATCH_STAT_EXSIT);
        CASE(ACCESS_ERROR_TEMPLATE_PARAM);
        CASE(ACCESS_ERROR_TEMPLATE_MATCH_BLACK);
        CASE(ACCESS_ERROR_TEMPLATE_MATCH_KEYWORD);
        CASE(ACCESS_ERROR_AUDIT_DONE);
        CASE(ACCESS_ERROR_AUDIT_FAIL);
        CASE(ACCESS_ERROR_ROUTER_FAIL);
        CASE(ACCESS_ERROR_ROUTER_PARAM);
        CASE(ACCESS_ERROR_ROUTER_NO_CHANNEL);
        CASE (ACCESS_ERROR_ROUTER_OVERFLOW);
        CASE(ACCESS_ERROR_OVERRATE_CHANNEL);
        CASE(ACCESS_ERROR_OVERRATE_CHARGE);
        CASE(ACCESS_ERROR_OVERRATE_OTHER);
        CASE(ACCESS_ERROR_NEXT_STATE);
        default:
            return "UNKOWN";
    }

    return "UNKOWN";

}

UInt64 getCurrentTimeMS()
{
    struct timeval timeNow;
    gettimeofday(&timeNow,NULL);
    return ( timeNow.tv_sec * 1000 + timeNow.tv_usec / 1000 ) ;
}

bool CommPostMsg( UInt32 uState, TMsg *pMsg )
{
    switch( uState )
    {
        case STATE_INIT:
            g_pSessionThread->PostMsg( pMsg );
            break;

        case STATE_TEMPLATE:
            g_pTemplateThread->PostMsg( pMsg );
            break;

        case STATE_AUDIT:
            g_pAuditThread->PostMsg( pMsg );
            break;

        case STATE_ROUTE:
            g_pRouterThread->PostMsg( pMsg );
            break;

        case STATE_OVERRATE:
            g_pOverRateThread->PostMsg( pMsg );
            break;

        default:
            return false;
    }

    return true;
}


