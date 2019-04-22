#include "TableInfo.h"
#include "Fmt.h"
#include "LogMacro.h"
#include "AppDataFactory.h"
#include "RuleLoadThread.h"

#include "boost/foreach.hpp"
#include "boost/date_time/posix_time/ptime.hpp"
#include "boost/date_time/posix_time/time_parsers.hpp"
#include "boost/date_time/posix_time/conversion.hpp"


#define foreach BOOST_FOREACH


#define CASE_TABLE_DATASETTYPE(tableid_, datasettype_) \
    case tableid_: \
    { \
        m_eDataSetType = datasettype_; \
        break; \
    }


#define SET_TBL_EXTRA(tableid_, datasettype_, sql_) \
    { \
        TableExtra t; \
        t.m_eDataSetType = datasettype_; \
        t.m_strSql = sql_; \
        m[tableid_] = t; \
    }


void clearAppData(AppDataVec& v)
{
    foreach (AppData* p, v)
    {
        SAFE_DELETE(p);
    }

    v.clear();
}

void clearAppData(AppDataMap& m)
{
    foreach (AppDataMapValueType iter, m)
    {
        AppData* p = iter.second;
        SAFE_DELETE(p);
    }

    m.clear();
}

void clearAppData(AppDataVecMap& mv)
{
    foreach (AppDataVecMapValueType iter, mv)
    {
        AppDataVec& v = iter.second;
        clearAppData(v);
        v.clear();
    }

    mv.clear();
}

void copyAppData(UInt32 uiTableId, AppDataVec& vecOld, AppDataVec& vecNew)
{
    foreach (AppData* pAppDataOld, vecOld)
    {
        CHK_NULL_RETURN(pAppDataOld);

        AppData* pAppDataNew = AppDataFactory::getInstance().newAppData(uiTableId);
        CHK_NULL_RETURN(pAppDataNew);

        pAppDataNew->copyData(pAppDataOld);
        vecNew.push_back(pAppDataNew);
    }
}

void copyAppData(UInt32 uiTableId, AppDataMap& mapOld, AppDataMap& mapNew)
{
    foreach (AppDataMapValueType iter, mapOld)
    {
        cs_t strKey = iter.first;
        AppData* pAppDataOld = iter.second;
        CHK_NULL_RETURN(pAppDataOld);

        AppData* pAppDataNew = AppDataFactory::getInstance().newAppData(uiTableId);
        CHK_NULL_RETURN(pAppDataNew);

        pAppDataNew->copyData(pAppDataOld);
        mapNew[strKey] = pAppDataNew;
    }
}

void copyAppData(UInt32 uiTableId, AppDataVecMap& mapVecOld, AppDataVecMap& mapVecNew)
{
    foreach (AppDataVecMapValueType iter, mapVecOld)
    {
        cs_t strKey = iter.first;
        AppDataVec& vecOld = iter.second;

        AppDataVec vecNew;
        copyAppData(uiTableId, vecOld, vecNew);

        mapVecNew[strKey] = vecNew;
    }
}


const TableInfo::TableExtraMap TableInfo::m_mapTblExtra = TableInfo::initTableExtra();


TableInfo::TableInfo()
{
    m_uiTableId = 0;
    m_uiUpdateTimestamp = 0;
    m_bUpdate = false;

    m_uiUpdateReqMsgId = 0;

    m_eDataSetType = DataSetType_Map;
    m_pFunc = NULL;
}

TableInfo::~TableInfo()
{
}

TableInfo::TableExtraMap TableInfo::initTableExtra()
{
    TableExtraMap m;

    SET_TBL_EXTRA(t_sms_agent_info, DataSetType_Map, "SELECT * FROM t_sms_agent_info WHERE status=1;");
    SET_TBL_EXTRA(t_sms_direct_client_give_pool, DataSetType_VecMap, "SELECT * FROM t_sms_direct_client_give_pool WHERE status=0 AND remain_number>0 AND due_time>SYSDATE() ORDER BY due_time,unit_price;");
    SET_TBL_EXTRA(t_sms_user_property_log, DataSetType_VecMap, "SELECT * FROM t_sms_user_property_log ORDER BY effect_date;");
    SET_TBL_EXTRA(t_sms_client_send_limit, DataSetType_Map, "SELECT * FROM t_sms_client_send_limit WHERE state=1;");

    return m;
}

void TableInfo::init()
{
    TableExtraMapCIter iter = m_mapTblExtra.find(m_uiTableId);

    if (m_mapTblExtra.end() == iter)
    {
        // 考虑到查找提高性能,默认使用的数据类型设置为Map
        // 但是,使用std::map时需要注意,std::map会根据key排序
        // 若sql带ORDER BY语法,则建议使用std::vector

        m_eDataSetType = DataSetType_Map;
        m_strSql = Fmt<64>("select * from %s;", m_strTableName);
    }
    else
    {
        const TableExtra& t = iter->second;
        m_eDataSetType = t.m_eDataSetType;
        m_strSql = t.m_strSql.empty()?Fmt<64>("select * from %s;", m_strTableName):t.m_strSql;
    }
}

void TableInfo::saveUpdateTime(cs_t s)
{
    m_bUpdate = false;

    if (19 != s.size())
    {
        LogError("Invalid time[%s].", s.data());
        return;
    }

    using namespace boost::posix_time;

    ptime pt(time_from_string(s));
    struct tm tm1 = to_tm(pt);
    time_t t = mktime(&tm1);
    UInt32 uiCurrentTimestamp = (-1 == t) ? 0 : t;

//    LogDebug("Current:%u, last:%u", uiCurrentTimestamp, m_uiUpdateTimestamp);

    if (uiCurrentTimestamp > m_uiUpdateTimestamp)
    {
        m_bUpdate = true;
        m_uiUpdateTimestamp = uiCurrentTimestamp;

        LogNotice("[%s:%u] update.", m_strTableName.data(), m_uiTableId);
    }
}

void TableInfo::insert(AppData* pAppData)
{
    switch (m_eDataSetType)
    {
        case DataSetType_Map:
        {
            m_mapAppData[pAppData->getKey()] = pAppData;
            break;
        }
        case DataSetType_Vec:
        {
            m_vecAppData.push_back(pAppData);
            break;
        }
        case DataSetType_VecMap:
        {
            const string& strKey = pAppData->getKey();
            AppDataVecMapIter iter = m_mapVecAppData.find(strKey);

            if (m_mapVecAppData.end() == iter)
            {
                vector<AppData*> v;
                v.push_back(pAppData);

                m_mapVecAppData[strKey] = v;
            }
            else
            {
                vector<AppData*>& v = iter->second;
                v.push_back(pAppData);
            }

            break;
        }
    }
}

void TableInfo::clear()
{
    clearAppData(m_vecAppData);
    clearAppData(m_mapAppData);
    clearAppData(m_mapVecAppData);
}

bool TableInfo::send2Thread()
{
    foreach (CThread* pThread, m_vecThread)
    {
        CHK_NULL_CONTINUE(pThread);

        DbUpdateReq* pReq = new DbUpdateReq();
        CHK_NULL_CONTINUE(pReq);

        pReq->m_uiTableId = m_uiTableId;
        pReq->m_strTableName = m_strTableName;
        pReq->copyData(this);
        pReq->m_iMsgType = MSGTYPE_RULELOAD_DB_UPDATE_REQ;

        pThread->PostMsg(pReq);

        LogNotice("Send table[%s] update message to thread[%s].",
            m_strTableName.data(),
            pThread->GetThreadName().data());
    }

    return true;
}


