#ifndef SMS_ACCESS_THREAD_COVERRATETHREAD_H
#define SMS_ACCESS_THREAD_COVERRATETHREAD_H

#include "Thread.h"
#include "SnManager.h"
#include "ChannelMng.h"
#include "PhoneDao.h"
#include "SignExtnoGw.h"
#include "ChannelSelect.h"
#include "ChannelScheduler.h"
#include "CoverRate.h"
#include "ChannelWhiteKeyword.h"
#include "searchTree.h"
#include "KeyWordCheck.h"
#include "enum.h"
#include "hiredis.h"

#include "boost/regex.hpp"
#include <regex.h>

#include <map>

using std::string;
using std::map;

////////////////////////////////////////////////////////////////////////////

bool startOverRateThread();

////////////////////////////////////////////////////////////////////////////

class OverRateCheckReqMsg : public TMsg
{
public:
    typedef enum OverRateCheckType
    {
        CommonCheck,
        ChannelOverRateCheck,
    }OverRateCheckType;

public:
    OverRateCheckReqMsg()
    {
        m_uiChannelId = 0;
        m_bIncludeChinese = false;
        m_eOverRateCheckType = CommonCheck;
        m_bInOverRateWhiteList = false;
    }

    ~OverRateCheckReqMsg() {}

private:
    OverRateCheckReqMsg(const OverRateCheckReqMsg&);

public:
    string m_strClientId;
    string m_strSmsId;
    string m_strPhone;
    string m_strSign;
    string m_strContent;
    string m_strSmsType;
    string m_strC2sDate;
    string m_strTemplateType;
    UInt32 m_uiChannelId;

    bool m_bIncludeChinese;
    bool m_bInOverRateWhiteList;

    OverRateCheckType m_eOverRateCheckType;
    
};

class OverRateCheckRspMsg : public TMsg
{
public:
    OverRateCheckRspMsg()
    {
        m_eOverRateResult = OverRateNone;
        m_ulOverRateSessionId = 0;
    }

    ~OverRateCheckRspMsg() {}

private:
    OverRateCheckRspMsg(const OverRateCheckRspMsg&);

public:
    // 标识如果超频了，是哪个流程超频
    OverRateResult m_eOverRateResult;

    // 超频错误码
    string m_strErrCode;

    // 超频线程会话ID
    UInt64 m_ulOverRateSessionId;

    //for channel overrate
    string m_strChannelOverRateKeyall;
};

class OverRateSetReqMsg : public TMsg
{
public:
    OverRateSetReqMsg()
    {
        m_uiState = 0;
    }
    ~OverRateSetReqMsg() {}

private:
    OverRateSetReqMsg(const OverRateSetReqMsg&);
    OverRateSetReqMsg& operator=(const OverRateSetReqMsg&);

public:
    // 0:后续流程成功，1:后续流程失败
    UInt32 m_uiState;
};

////////////////////////////////////////////////////////////////////////////

class OverRateThread : public CThread
{
class Session
{
public:
typedef enum CheckOverRateState
{
    CheckGlobalOverRate,
    CheckKeywordOverRate,
    CheckSmsTypeOverRate,
    CheckChannelOverRate,
    CheckOverRateOk,
}CheckOverRateState;

public:
    Session(OverRateThread* pThread);
    ~Session();

    void setOverRateCheckReqMsg(const OverRateCheckReqMsg* pReq);

private:
    void init();

public:
    OverRateThread* m_pThread;

    UInt64 m_ulSequence;

    OverRateCheckReqMsg m_reqMsg;

    CheckOverRateState m_eCheckOverRateState;

    // 超频规则
    models::TempRule m_globalOverRateRule;
    models::TempRule m_keywordOverRateRule;
    models::TempRule m_smsTypeOverRateRule;
    models::TempRule m_channelOverRateRule;

    // 超频key
    string m_strKeyWordOverRateKey;
    string m_strSmsTypeOverRateKey;
    string m_strChannelOverRateKey;

    // 超频KeyType
    int m_iKeyWordOverRateKeyType;

    // 是否在超频白名单中
    bool m_bInOverRateWhiteList;

    OverRateResult m_eOverRateResult;

    string m_strErrorCode;

    OverRateResult m_eOverRateKeywordOrSmsType;

    string m_strCurrentDay;
    string m_strChannelOverRateKeyall;
};

typedef map<UInt64, Session*> SessionMap;
typedef SessionMap::iterator SessionMapIter;



    /* 超频redis字符串的长度*/
    #define SMS_OVERRATE_REDIS_STRING_MAX_SIZE  ( 1024*1024 )

    typedef vector<string> VecString;
    typedef list<regex> RegList;

    typedef std::list<std::string> stl_list_str;
    typedef std::list<Int32> stl_list_int32;


public:
    OverRateThread(const char *name);
    ~OverRateThread();

    bool Init();

    UInt32 GetSessionMapSize() {return m_mapSession.size();}
	void smsChannelDayOverRateDescRedis(string strKey, int value);

private:
    virtual void MainLoop();
    virtual void HandleMsg(TMsg* pMsg);

    void initAuditOverKeyWordSearchTree(map<string, list<string> >& mapSetIn, map<string, searchTree*>& mapSetOut);

    void handleOverRateCheckReqMsg(TMsg* pMsg);
    void checkOverRate(Session* pSession);
    void checkGlobalOverRate(Session* pSession);
    void checkKeywordOverRate(Session* pSession);
    void checkSmsTypeOverRate(Session* pSession);
    void checkChannelOverRate(Session* pSession);


    void handleRedisRspMsg(TMsg* pMsg);
    void checkRedisGlobalOverRate(Session* pSession, redisReply* pRedisReply);
    bool checkRedisGlobalOverRateEx(Session* pSession, void** data, int size);

    void checkRedisKeywordOverRate(Session* pSession, redisReply* pRedisReply);
    bool checkRedisKeywordOverRateEx(Session* pSession, string& strtimes);

    void checkRedisSmsTypeOverRate(Session* pSession, redisReply* pRedisReply);
    bool checkRedisSmsTypeOverRateEx(Session* pSession, string& strtimes);

    void checkRedisChannelDayOverRate(Session* pSession, redisReply* pRedisReply);
    bool checkRedisChannelDayOverRateEx(Session* pSession, void** data, int size);

    void checkRedisChannelOverRate(Session* pSession, redisReply* pRedisReply);
    bool checkRedisChannelOverRateEx(Session* pSession, string& strtimes);
    void checkRedisChannelOverRateSetRedis(Session* pSession, cs_t redisStr, bool bReset);

    void increaseRedisGlobalOverRate(Session* pInfo, int value);
    void decreaseRedisGlobalOverRate(Session* pInfo);
    bool OverRateKeywordRegex(Session* pInfo, string& strOut);


    void handleOverRateSetReqMsg(TMsg* pMsg);
    void setKeywordOverRateRedis(Session* pSession);
    void setSmsTypeOverRateRedis(Session* pSession);
    void setChannelOverRateRedis(Session* pSession);
    void setOverRateRedis(cs_t strRedisCmdKey, cs_t redisStr, bool bReset);

    void handleTimeout(TMsg* pMsg);

    void getKeyWordOverRateKey(Session* pSession);
    void getSmsTypeOverRateKey(Session* pSession);

    void sendOverRateCheckRspMsg(Session* pSession);
    void sendOverRateCheckRspMsgEx(Session* pSession);

private:
    SessionMap m_mapSession;

    SnManager m_snManager;

    map<string, searchTree*> m_OverRateKeyWordMap;

    ChannelSelect *m_chlSelect;

    CoverRate *m_pOverRate;

    CThreadWheelTimer* m_pCleanOverRateTimer;

    set<string> m_overRateWhiteList;

};

extern OverRateThread* g_pOverRateThread;

#endif // SMS_ACCESS_THREAD_COVERRATETHREAD_H
