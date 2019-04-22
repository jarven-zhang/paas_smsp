#ifndef __C_BLACKLIST_THREAD_H__
#define __C_BLACKLIST_THREAD_H__

#include "Thread.h"
#include "CommClass.h"
#include "hiredis.h"
#include "CSessionThread.h"
#include "BlackList.h"

class CBlackListThread : public CThread
{

public:
    CBlackListThread(const char* name );
    ~CBlackListThread();

public:
    bool   Init();
    UInt32 GetRedisBlackListInfo( session_pt pSession );
    UInt32 GetBlackTimeOut(TMsg* pMsg, session_pt pSession );
    void   HandleMsg( TMsg* pMsg );
    bool   CheckBlacklistRedisResp( redisReply* pRedisReply, session_pt pSession );

private:
    bool   CheckSysBlackList( session_pt pSession );
    bool   ProUserBlackList( session_pt pSession );
    bool   ProBlacklistClassify(session_pt pSession);
    bool   CheckBlacklistClassify(session_pt pSession, UInt32 uBLGrp);
    void   GetSysPara( const STL_MAP_STR& mapSysPara );

private:
    bool                m_bBlankPhoneSwitch;// 空号库检查开关
    map<UInt32,UInt64>  m_uBgroupRefCategoryMap;
    map<string,UInt32>  m_uBlacklistGrpMap;
    USER_GW_MAP         m_userGwMap;

};

#endif
