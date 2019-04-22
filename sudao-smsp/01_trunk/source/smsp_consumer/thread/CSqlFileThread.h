#ifndef __C_SQLFILETHREAD_H__
#define __C_SQLFILETHREAD_H__

#include <string.h>
#include <time.h>
#include "Thread.h"


class TSqlFileReq : public TMsg
{
public:
    std::string m_strSqlContent;
};


class CSqlFileThread : public CThread
{
public:
    CSqlFileThread(const char *name);
    ~CSqlFileThread();

    bool Init(const std::string name, const std::string fileDir);
    
private:
    virtual void HandleMsg(TMsg* pMsg);
    bool OpenFile(const std::string name, std::string fileDir);
    void WriteFile(TMsg* pMsg);
    void NewHourFile();

    FILE* m_pSqlFile;
    std::string m_strSqlfileDir;
    std::string m_strPrefixName;
    tm m_CurrentTime;
};

#endif  //__C_SQLFILETHREAD_H__

