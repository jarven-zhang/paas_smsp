#ifndef __SMS_ACCOUNT_H__
#define __SMS_ACCOUNT_H__
#include <string>
#include <list>
#include <set>
#include <map>
#include <iostream>
#include "platform.h"
#include "BusTypes.h"

using namespace std;

enum HttpProtocolType
{
    HttpProtocolType_Json               = 0,    // for universal
    HttpProtocolType_Get_Post           = 1,
    HttpProtocolType_TX_Json            = 2,    // for tencent cloud
    HttpProtocolType_JD                 = 3,
    HttpProtocolType_JD_Webservice      = 4,    // for jing dong
    HttpProtocolType_ML                 = 5,
};
#define ACCOUNT_EXT_PORT_TO_SIGN        0x01  /* convert sign port to sign */
#define ACCOUNT_EXT_MERGE_LONG_REDIS    0x02  /* merge long msg in redis  */
#define ACCOUNT_EXT_HTTPS_REPORT_RETRY  0x08  /* https report repush  */

class SmsAccount
{
public:
    SmsAccount();
    virtual ~SmsAccount();

public:
    string m_strAccount;
    string m_strPWD;
    string m_strname;  ///// user name
    ////float  m_fFee;
    string m_strSid;
    UInt32 m_uStatus; // 0:no register   1:register ok,5:freeze,6:cancel,7:locked
    UInt32 m_uNeedReport; //0：no，  1：yes,simpleReport        2:YES,SourceReport
    UInt32 m_uNeedMo;     //upstream    0：no，1：yes
    UInt32 m_uSupportLongMo; //0:default not support ,means long mo will split;  1:support
    UInt32 m_uNeedaudit;  // 0：no，1：yes
    string m_strCreattime;
    string m_strIP;       /////ip white list
    string m_strMoUrl;    //// mo url
    UInt32 m_uNodeNum;    //// connect num
    UInt32 m_uPayType;       // 0: 预付费    1:后付费 2:扣主账户 3:扣子账户
    UInt32 m_uNeedExtend;
    UInt32 m_uNeedSignExtend;
    string m_strDeliveryUrl;   //// RT url

    UInt64 m_uAgentId;
    string m_strSmsType;
    UInt32 m_uSmsFrom;
    UInt32 m_uOverRateCharge;
    string m_strSpNum;

    UInt32 m_uLoginErrorCount;
    UInt32 m_uLinkCount;
    set<string> m_setLinkId;
    UInt64 m_uLockTime;
    bool   m_bAllIpAllow;
    list<string> m_IPWhiteList;

    UInt8 loginMode;			//only smgp
    UInt32 m_uGetRptInterval;
    UInt32 m_uGetRptMaxSize;

    ////under SGIP only
    string m_strSgipRtMoIp;
    UInt16 m_uSgipRtMoPort;
    UInt32 m_uSgipNodeId;
    UInt32 m_uIdentify;		//index of t_sms_access_*
    UInt32 m_uClientLeavel;
    UInt32 m_uSignPortLen;		///sign port length
    string m_strExtNo;			///user port
    UInt32 m_uClientAscription;			///add by weilu for cloud charge
    UInt64 m_uBelong_sale;  ///this account is belong to which saler

    int m_iPoolPolicyId;       // 通道池策略ID
    UInt8 m_ucHttpProtocolType;
    UInt8 m_uFailedResendFlag;  //0:not support resend,  1: support resend
    UInt32 m_uAccountExtAttr;
    UInt32 m_uGroupsendLimCfg;
    UInt32 m_uGroupsendLimUserflag;
    UInt32 m_uGroupsendLimNum;
    UInt32 m_uGroupsendLimTime;

    UInt32 m_uiSpeed;
};

typedef map<string, SmsAccount> SmsAccountMap;
typedef SmsAccountMap::iterator SmsAccountMapIter;

#endif  /*__SMS_ACCOUNT_H__*/
