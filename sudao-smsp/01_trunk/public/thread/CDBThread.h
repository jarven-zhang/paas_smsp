
#ifndef _CDBThread_h_
#define _CDBThread_h_

#include <string.h>
#include <mysql.h>
#include "Thread.h"
#include "CThreadSQLHelper.h"


#define CHECK_DB_CONN_TIMEOUT       3*1000   //3s check db conn.
#define CHECK_DB_CONN_TIMER_MSGID   3080

class TSqlContentReq : public TMsg
{
public:
    std::string m_strSqlContent;
};

class TDBQueryReq: public TMsg
{
public:
    std::string m_SQL;
	std::string m_strKey;	//for hash chose link
};

class TDBQueryResp: public TMsg
{
public:
	TDBQueryResp()
	{
		m_iResult = 0;
		m_iAffectRow = 0;
	}
    int m_iResult;
	int m_iAffectRow;		//affectRow
	std::string m_strKey;
    RecordSet *m_recordset;
};

typedef enum OperationFlag_E
{
    OperFlag_Insert     = 0,
    OperFlag_Update     = 1,
    OperFlag_Invalid
}OperFlag;

typedef enum TableFlag_E
{
    TableFlag_Access     = 0,   // t_sms_access_x_xxxxxxxx
    TableFlag_Record     = 1,   // t_sms_record_x_xxxxxxxx
    TableFlag_Invalid
}TableFlag;

class SessionInfo
{
public:
    SessionInfo()
    {
        eOperFlag_      = OperFlag_Invalid;
        eTableFlag_     = TableFlag_Invalid;
        ucState_        = INVALID_UINT8;
        usReQueueTimes_ = 0;
        uiReQueueTime_  = 0;
        bModify_        = false;
        uiStatePos_     = 0;
    }

public:
    string      strSrcSql_;
    string      strId_;
    OperFlag    eOperFlag_;
    TableFlag   eTableFlag_;
    UInt16      usReQueueTimes_;
    UInt32      uiReQueueTime_;
    UInt8       ucState_;
    string      strSqlModifyWhere_;
    string      strSqlModifySet_;
    bool        bModify_;

    string::size_type      uiStatePos_;

private:
    SessionInfo(const SessionInfo&);
    // SessionInfo& operator=(const SessionInfo& s);
};

class DBInReq : public TMsg
{
public:
    SessionInfo sessionInfo_;

private:
    // DBInReq(const DBInReq&);
    // DBInReq& operator=(const DBInReq&);
};


class CDBThread:public CThread
{

public:
    CDBThread(const char *name);
    ~CDBThread();
    bool Init(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname);

	bool Available();
	bool DBPing();
	void mysqlFailedAlarm(int iCount, int type);

	MYSQL* CDBGetConn()
	{
		return m_DBConn;
	}

private:

    virtual void HandleMsg(TMsg* pMsg);
	void DBUpdateReq(TMsg* pMsg);
    void DBQueryReq(TMsg* pMsg);
    void DBNotQueryReq(TMsg* pMsg);
	bool DBConnect(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname);

    void GetSysPara(const std::map<std::string, std::string>& mapSysPara);

    void handleDbInReq(TMsg* pMsg);
    void handleDbInReqForRecord(const SessionInfo& s);
    void handleDbInReqForAccess(const SessionInfo& s);

    int excuteSql(const string& strSql, int& err);
    void reQueue(const SessionInfo& s);

private:
    MYSQL* m_DBConn;
	CThreadWheelTimer *m_pTimer;

	std::string m_DBHost;
	std::string m_DBUser;
	std::string m_DBPwd;
	std::string m_DBName;
	int m_DBPort;
	UInt32 m_uDBExecultErrCunts;
	UInt64 m_componentId;

    UInt16 m_usReQueueTimes;
    UInt32 m_uiReQueueTime;
	UInt32 m_uDBSwitch;
	UInt32 m_uifWriteBigData;	
};


//for mysql alarm
enum
{
	MYSQL_CONNECT_FAIL = 0,
	MYSQL_EXE_COMMAND_FAIL,
};



#endif

