#ifndef _CLogThread_h_
#define _CLogThread_h_

#include "Thread.h"
#include "LogMacro.h"

enum Level
{
    LEVEL_DEBUG,
    LEVEL_INFO,
    LEVEL_NOTICE,
    LEVEL_ELK,
    LEVEL_WARN,
    LEVEL_ALERT,
    LEVEL_ERROR,
    LEVEL_CRIT,
    LEVEL_EMERG,
    LEVEL_FATAL
};

#define MAX_LOG_LENGTH 4096

extern void Log(Level level, const char *file, const int line, const char *func, const char *format, ...);
extern void LogP(Level level, const char* file, const int line, const char* func, const char* format, ...);
extern void LogT(Level level, cs_t strThreadName, const char *file, const int line, const char *func, const char *format, ...);


class TLogReq: public TMsg
{
public:
    std::string m_LogContent;
};

class TLogResp: public TMsg
{
public:
    int m_iResult;
};

class CLogThread:public CThread
{
public:
    CLogThread(const char *name);
    ~CLogThread();

    bool Init(const std::string name, const std::string LogFileDir);
    bool OpenFile(const std::string name, std::string LogFileDir);
    int GetLevel();
    void SetLevel(Level level);

private:
    virtual void MainLoop();
    virtual void HandleMsg(TMsg* pMsg);

    void WriteFile(TMsg* pMsg);
    void NewDayLogFile();

    FILE *m_LogFile;
    int m_iLogLevel;
    std::string m_sLogfileDir;
    std::string m_sPrefixName;
    tm m_CurrentTime;
};

extern CLogThread* g_pLogThread;

#endif

