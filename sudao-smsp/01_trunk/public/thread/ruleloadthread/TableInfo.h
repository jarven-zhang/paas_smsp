#ifndef THREAD_RULELOADTHREAD_TABLEINFO_H
#define THREAD_RULELOADTHREAD_TABLEINFO_H

#include "platform.h"
#include "AppData.h"

#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;


void clearAppData(AppDataVec& v);
void clearAppData(AppDataMap& m);
void clearAppData(AppDataVecMap& mv);

void copyAppData(UInt32 uiTableId, AppDataVec& vecOld, AppDataVec& vecNew);
void copyAppData(UInt32 uiTableId, AppDataMap& mapOld, AppDataMap& mapNew);
void copyAppData(UInt32 uiTableId, AppDataVecMap& mapVecOld, AppDataVecMap& mapVecNew);


class CThread;
class RuleLoadThread;


class TableInfo
{
    enum DataSetType
    {
        DataSetType_Map,
        DataSetType_Vec,
        DataSetType_VecMap
    };

    struct TableExtra
    {
        DataSetType m_eDataSetType;
        string m_strSql;

        TableExtra():m_eDataSetType(DataSetType_Map){}
    };

    typedef bool (RuleLoadThread::*func)();
    typedef map<UInt32, TableExtra> TableExtraMap;
    typedef map<UInt32, TableExtra>::const_iterator TableExtraMapCIter;

public:
    TableInfo();
    ~TableInfo();

    static TableExtraMap initTableExtra();

    void init();

    void saveUpdateTime(cs_t s);

    void insert(AppData* pAppData);

    void clear();

    void copy(AppDataVec& v) {copyAppData(m_uiTableId, m_vecAppData, v);}
    void copy(AppDataMap& m) {copyAppData(m_uiTableId, m_mapAppData, m);}
    void copy(AppDataVecMap& mv) {copyAppData(m_uiTableId, m_mapVecAppData, mv);}

    bool send2Thread();

private:
    TableInfo(const TableInfo&);
    TableInfo& operator=(const TableInfo&);

//    void initSql();
//
//    void initDataSetType();

public:
    static const TableExtraMap m_mapTblExtra;

    // 表ID
    UInt32 m_uiTableId;

    // 表名称
    string m_strTableName;

    // 表更新时间戳
    UInt32 m_uiUpdateTimestamp;

    // 是否更新
    bool m_bUpdate;

    // 表更新消息ID
    UInt32 m_uiUpdateReqMsgId;

    // 注册线程集合
    vector<CThread*> m_vecThread;

    // 回调函数
    func m_pFunc;

    // select sql
    string m_strSql;

    // 数据集合类型
    DataSetType m_eDataSetType;

    // 数据集合
    AppDataMap m_mapAppData;
    AppDataVec m_vecAppData;
    AppDataVecMap m_mapVecAppData;
};

typedef map<UInt32, TableInfo*> TblMgrMap;
typedef map<UInt32, TableInfo*>::const_iterator TblMgrMapCIter;
typedef map<UInt32, TableInfo*>::value_type TblMgrMapValueType;

#endif // THREAD_RULELOADTHREAD_TABLEINFO_H