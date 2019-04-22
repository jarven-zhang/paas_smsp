#ifndef __ACCESS_CHARGE_THREAD_H__
#define __ACCESS_CHARGE_THREAD_H__

#include "Thread.h"
#include "CommClass.h"
#include "internalservice.h"
#include "SmspUtils.h"
#include "CSessionThread.h"

using namespace std;
using namespace models;

class CChargeThread : public CThread
{

public:
    CChargeThread(const char* name);
    ~CChargeThread();
    bool Init();
    UInt32 GetSessionMapSize();

public:
    void smsChargeTimeOut(TMsg* pMsg);
    bool smsChargePre(session_pt pSession);
    bool smsChargePost(session_pt pSession);
    UInt32 smsChargeResult(session_pt pSession, UInt32 uResult, const string& strContent);
    bool smsChargeSelect();

    void HandleMsg(TMsg* pMsg);

private:
    InternalService* m_pInternalService;
    UInt64           m_uAccessId;
    string           m_strChargeUrl;
    SmspUtils        m_SmspUtils;
    agentInfoMap     m_AgentInfoMap;
    agentAcctMap     m_AgentAcct;
    accountMap       m_AccountMap;
};

#endif
