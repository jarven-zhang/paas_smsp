#ifndef THREAD_RULELOADTHREAD_RULELOADTHREAD_H
#define THREAD_RULELOADTHREAD_RULELOADTHREAD_H

#include "platform.h"
#include "TableInfo.h"
#include "TableId.h"
#include "LogMacro.h"

#include <string>
#include <map>

using std::string;
using std::map;


#define GET_RULELOAD_DATA_MAP(tableid_, map_) \
        INIT_CHK_FUNC_RET_FALSE(g_pRuleLoadThreadV2->getData<AppDataMap>((CThread*)this, tableid_, VNAME(tableid_), map_));

#define GET_RULELOAD_DATA_VEC(tableid_, vec_) \
        INIT_CHK_FUNC_RET_FALSE(g_pRuleLoadThreadV2->getData<AppDataVec>((CThread*)this, tableid_, VNAME(tableid_), vec_));

#define GET_RULELOAD_DATA_VECMAP(tableid_, map_) \
        INIT_CHK_FUNC_RET_FALSE(g_pRuleLoadThreadV2->getData<AppDataVecMap>((CThread*)this, tableid_, VNAME(tableid_), map_));


#define CASE_UPDATE_TABLE_VEC(tableid_, vec_) CASE_UPDATE_TABLE_VEC_FUNC(tableid_, vec_, int a=1;a=a;)

#define CASE_UPDATE_TABLE_VEC_FUNC(tableid_, vec_, func_) \
        case tableid_: \
        { \
            clearAppData(vec_); \
            vec_ = pReq->m_vecAppData; \
            LogNotice("update %s:%u. size[%u].", VNAME(tableid_), tableid_, vec_.size()); \
            func_; \
            break; \
        }


#define CASE_UPDATE_TABLE_MAP(tableid_, map_) CASE_UPDATE_TABLE_MAP_FUNC(tableid_, map_, int a=1;a=a;)

#define CASE_UPDATE_TABLE_MAP_FUNC(tableid_, map_, func_) \
        case tableid_: \
        { \
            clearAppData(map_); \
            map_ = pReq->m_mapAppData; \
            LogNotice("update %s:%u. size[%u].", VNAME(tableid_), tableid_, map_.size()); \
            func_; \
            break; \
        }


#define CASE_UPDATE_TABLE_VECMAP(tableid_, mapVec_) CASE_UPDATE_TABLE_VECMAP_FUNC(tableid_, mapVec_, int a=1;a=a;)

#define CASE_UPDATE_TABLE_VECMAP_FUNC(tableid_, mapVec_, func_) \
        case tableid_: \
        { \
            clearAppData(mapVec_); \
            mapVec_ = pReq->m_mapVecAppData; \
            LogNotice("update %s:%u. size[%u].", VNAME(tableid_), tableid_, mapVec_.size()); \
            func_; \
            break; \
        }


class DbUpdateReq : public TMsg
{
public:
    DbUpdateReq():m_uiTableId(0) {}
    ~DbUpdateReq() {}

    void copyData(TableInfo* pTbl)
    {
        CHK_NULL_RETURN(pTbl);

        copyAppData(pTbl->m_uiTableId, pTbl->m_vecAppData, m_vecAppData);
        copyAppData(pTbl->m_uiTableId, pTbl->m_mapAppData, m_mapAppData);
        copyAppData(pTbl->m_uiTableId, pTbl->m_mapVecAppData, m_mapVecAppData);
    }

private:
    DbUpdateReq(DbUpdateReq&);
    DbUpdateReq& operator=(DbUpdateReq&);

public:
    UInt32 m_uiTableId;
    string m_strTableName;

    AppDataVec m_vecAppData;
    AppDataMap m_mapAppData;
    AppDataVecMap m_mapVecAppData;
};


class RuleLoadThread : public CThread
{
    typedef map<string, UInt32> TblName2IdMap;

public:
    RuleLoadThread(cs_t strDbHost,
                        UInt16 usDbPort,
                        cs_t strDbUserName,
                        cs_t strDbPassword,
                        cs_t strDatabaseName);
    ~RuleLoadThread();

    bool Init();

    template<typename T>
    bool getData(CThread* pThread, UInt32 uiTblId, cs_t strTblName, T& t)
    {
        if (!regTbl(pThread, uiTblId, strTblName))
        {
            LogErrorP("[%s:%u] Call regTbl failed.", strTblName.data(), uiTblId);
            return false;
        }

        if (!getTblDataFromDb(uiTblId))
        {
            LogErrorP("[%s:%u] Call getTblDataFromDb failed.", strTblName.data(), uiTblId);
            return false;
        }

        TblMgrMapCIter iter;
        CHK_MAP_FIND_UINT_RET_FALSE(m_mapTblMgr, iter, uiTblId);

        TableInfo* pTbl = iter->second;
        INIT_CHK_NULL_RET_FALSE(pTbl);

        pTbl->copy(t);

        LogDebug("[%s:%u] %s get %u data.",
            strTblName.data(), uiTblId, pThread->GetThreadName().data(), t.size());

        return true;
    }

private:
    RuleLoadThread(const RuleLoadThread&);
    RuleLoadThread& operator=(const RuleLoadThread&);

    bool connect();
    bool pingDb();

    virtual void HandleMsg(TMsg* pMsg);
    bool checkDbUpdate();

    bool regTbl(CThread* pThread, UInt32 uiTblId, cs_t strTblName);
    bool getTblDataFromDb(UInt32 uiTblId);
    bool saveUpdateTime();
    bool send2Thread();

private:
    string m_strDbHost;
    UInt16 m_usDbPort;
    string m_strDbUserName;
    string m_strDbPassword;
    string m_strDatabaseName;

    MYSQL* m_pDbConn;

    CThreadWheelTimer* m_pTimerDbUpdate;
    CThreadWheelTimer* m_pTimerDbConn;

    TblMgrMap m_mapTblMgr;
    TblName2IdMap m_mapTblName2Id;
};

extern RuleLoadThread* g_pRuleLoadThreadV2;

#endif // THREAD_RULELOADTHREAD_RULELOADTHREAD_H