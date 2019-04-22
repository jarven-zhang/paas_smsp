#ifndef _CDBThreadPOOL_h_
#define _CDBThreadPOOL_h_

#include <string.h>
#include "Thread.h"
#include "CThreadSQLHelper.h"

#include <mysql.h>

class CDBThread;


const UInt32 DBTHREAD_MAXNUM = 16;

class CDBThreadPool:public CThread
{

public:
    CDBThreadPool(const char *name, UInt32 uThreadNum);
    ~CDBThreadPool();
    bool Init(cs_t dbhost, unsigned int dbport, cs_t dbuser, cs_t dbpwd, cs_t dbname);

    int GetMsgSize();
    UInt32 GetMsgCount();
    void SetMsgCount(int count);
    virtual bool PostMsg(TMsg* pMsg);

    MYSQL* CDBGetConn();

    UInt32 getThreadNum() const {return m_uThreadNum;}
    CDBThread* getThread(UInt32 uiIdx) const;

private:
    virtual void HandleMsg(TMsg* pMsg);

private:
    UInt32 m_uThreadNum;
    UInt32 m_iIndex;
    CDBThread* m_pThreadNode[DBTHREAD_MAXNUM];
};

#endif
