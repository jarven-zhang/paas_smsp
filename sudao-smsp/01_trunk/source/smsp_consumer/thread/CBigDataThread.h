#ifndef __C_BIGDATATHREAD_H__
#define __C_BIGDATATHREAD_H__

#include <string.h>
#include <time.h>
#include "Thread.h"


class CBigDataThread : public CThread
{
public:
    CBigDataThread(const char *name);
    ~CBigDataThread();

    bool Init(const std::string name, const std::string fileDir);
    
private:
    virtual void HandleMsg(TMsg* pMsg);
    bool OpenFile(const std::string name, std::string fileDir);
    void WriteFile(TMsg* pMsg);
    void NewDayLogFile();

    FILE* m_pSqlFile;
    std::string m_strSqlfileDir;
    std::string m_strPrefixName;
    tm m_CurrentTime;
};

#endif  //__C_BIGDATATHREAD_H__

