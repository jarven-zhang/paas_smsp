#include "CLogThread.h"
#include "CRuleLoadThread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>

#include <iostream>

const std::string LEVEL_NAME[] =
{
    "DEBUG",
    "INFO",
    "NOTICE",
    "ELK",
    "WARN",
    "ALERT",
    "ERROR",
    "CRIT",
    "EMERG",
    "FATAL"
};

const std::string LEVEL_COLOR[] =
{
    "\033[0;36m",/* clan */
    "\033[01;34m",/* light blue */
    "\033[01;34m",/* light blue */
    "\033[01;33m",/* light yellow */
    "\033[01;32m",/* light green */
    "\033[01;31m",/* light red */
    "\033[01;31m",/* light red */
    "\033[01;31m",/* light red */
    "\033[01;35m",/* light magenta */
    "\033[01;35m" /* light magenta */
};

//CLogThread* g_pLogThread = NULL;

void LogP(Level level, const char* file, const int line, const char* func, const char* format, ...)
{
    tm ptm = {0};
    time_t now = time(NULL);
    localtime_r((const time_t*) &now, &ptm);
    char ctime[25] = {0};

    snprintf(ctime, sizeof(ctime), "[%04d-%02d-%02d][%02d:%02d:%02d]",
        ptm.tm_year + 1900,
        ptm.tm_mon + 1,
        ptm.tm_mday,
        ptm.tm_hour,
        ptm.tm_min,
        ptm.tm_sec);

    ////////////////////////////////////////////////////////////////////////////////////

    const std::string& levelName = LEVEL_NAME[level];
    std::string strClour = LEVEL_COLOR[level];

    char strLog[MAX_LOG_LENGTH + 1] = {0};
    char filename[128] = {0};

    char* p = strrchr(const_cast<char*>(file), '/');

    if (p == NULL)
    {
        snprintf(filename, 128, "%s", file);
    }
    else
    {
        snprintf(filename, 128, "%s", p + 1);
    }

    snprintf(strLog, MAX_LOG_LENGTH, "%s[%s][%s:%d][%s] ", strClour.data(), levelName.data(), filename, line, func);

    va_list argp;
    va_start(argp, format);
    vsnprintf(strLog + strlen(strLog), MAX_LOG_LENGTH - strlen(strLog), format, argp);
    va_end(argp);

    string strLogContent;
    strLogContent.append(ctime);
    strLogContent.append(strLog);
    strLogContent.append("\033[0m");
    printf("%s\n", strLogContent.data());

    if (NULL != g_pLogThread)
    {
        TLogReq* pReq = new TLogReq();
        pReq->m_LogContent.append(strLog);
        pReq->m_LogContent.append("\033[0m");
        pReq->m_iMsgType = MSGTYPE_LOG_REQ;
        g_pLogThread->PostMsg(pReq);
    }
}

void Log(Level level, const char *file, const int line, const char *func, const char *format, ...)
{
    const std::string &levelName = LEVEL_NAME[level];
    std::string strClour = LEVEL_COLOR[level];

    char strLog[MAX_LOG_LENGTH + 1] = {0};
    char filename[128] = {0};

    char *p = strrchr(const_cast<char*>(file), '/');
    if(p == NULL)
    {
        snprintf(filename, 128,"%s", file);
    }
    else
    {
        snprintf(filename, 128,"%s", p + 1);
    }
    snprintf(strLog, MAX_LOG_LENGTH,"%s[%s][%s:%d][%s] ",  strClour.data(), levelName.data(), filename, line, func);

    va_list argp;
    va_start(argp,format);
    /////vsnprintf(&strLog[strlen(strLog)], MAX_LOG_LENGTH - strlen(strLog), format, argp);
    vsnprintf(strLog+strlen(strLog), MAX_LOG_LENGTH - strlen(strLog), format, argp);
    va_end(argp);

    TLogReq* pReq = new TLogReq();
    pReq->m_LogContent = strLog;
    pReq->m_LogContent.append("\033[0m");
    pReq->m_iMsgType   = MSGTYPE_LOG_REQ;
    g_pLogThread->PostMsg(pReq);
}

void LogT(Level level, cs_t strThreadName, const char *file, const int line, const char *func, const char *format, ...)
{
    const std::string &levelName = LEVEL_NAME[level];
    std::string strClour = LEVEL_COLOR[level];

    char strLog[MAX_LOG_LENGTH + 1] = {0};
    char filename[128] = {0};

    char *p = strrchr(const_cast<char*>(file), '/');
    if(p == NULL)
    {
        snprintf(filename, 128,"%s", file);
    }
    else
    {
        snprintf(filename, 128,"%s", p + 1);
    }

    snprintf(strLog, MAX_LOG_LENGTH,"%s[%s][%s:%d][%s][%s] ",
        strClour.data(),
        levelName.data(),
        filename,
        line,
        strThreadName.data(),
        func);

    va_list argp;
    va_start(argp,format);
    vsnprintf(strLog+strlen(strLog), MAX_LOG_LENGTH - strlen(strLog), format, argp);
    va_end(argp);

    TLogReq* pReq = new TLogReq();
    pReq->m_LogContent = strLog;
    pReq->m_LogContent.append("\033[0m");
    pReq->m_iMsgType   = MSGTYPE_LOG_REQ;
    g_pLogThread->PostMsg(pReq);
}

CLogThread::CLogThread(const char *name):
    CThread(name)
{
    m_LogFile   = NULL;
    // LOG_LEVEL default: 2
    // DEBUG = 0; INFO = 1; NOTICE = 2; WARN = 3; ALERT = 4; ERROR = 5; CRIT = 6; EMERG = 7; FATAL = 8;
    m_iLogLevel = LEVEL_NOTICE;
}

CLogThread::~CLogThread()
{

}

int CLogThread::GetLevel()
{
    int level;
    pthread_mutex_lock(&m_mutex);
    level = m_iLogLevel;
    pthread_mutex_unlock(&m_mutex);
    return level;
}

void CLogThread::SetLevel(Level level)
{
    if (level <= LEVEL_FATAL)
    {
        pthread_mutex_lock(&m_mutex);
        m_iLogLevel = level;
        pthread_mutex_unlock(&m_mutex);
    }
}

bool CLogThread::Init(const std::string PrefixName, std::string LogFileDir)
{
    if(false == OpenFile(PrefixName, LogFileDir))
    {
        printf("OpenFile is failed.\n");
        return false;
    }
    if(false == CThread::Init())
    {
        printf("CThread::Init is failed.\n");
        return false;
    }
    return true;
}

void CLogThread::MainLoop()
{
    while(1)
    {
        pthread_mutex_lock(&m_mutex);
        TMsg* pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if (NULL == pMsg)
        {
            usleep(1000*10);
        }
        else
        {
            HandleMsg(pMsg);
            delete pMsg;
        }
    }
}

void CLogThread::HandleMsg(TMsg* pMsg)
{
    if (NULL == pMsg)
    {
        return;
    }

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    switch (pMsg->m_iMsgType)
    {
        case  MSGTYPE_LOG_REQ:
        {
            WriteFile(pMsg);
            break;
        }
        case  MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
        {
            int level;
            TUpdateSysParamRuleReq* pSysParamReq = (TUpdateSysParamRuleReq*)pMsg;
            std::map<std::string, std::string>::iterator it;
            it = pSysParamReq->m_SysParamMap.find("LOG_LEVEL");
            if(pSysParamReq->m_SysParamMap.end() != it)
            {
                level = atoi(it->second.data());
                if(level != GetLevel())
                    SetLevel((Level)level);
            }

            break;
        }
        default:
        {
            break;
        }
    }
}

bool CLogThread::OpenFile(const std::string PrefixName, const std::string LogFileDir)
{
    if (NULL == m_LogFile )
    {
        tm ptm = {0};
        time_t now = time(NULL);
        std::string fileName;
        std::string fullPath;
        std::string strLink;
        localtime_r((const time_t *) &now,&ptm);
        memcpy(&m_CurrentTime, &ptm, sizeof(tm));
        char szTime[60] = { 0 };
        snprintf(szTime, 60,"%s_%04d%02d%02d", PrefixName.data(), (1900 + ptm.tm_year),
                (1 + ptm.tm_mon), ptm.tm_mday);
        fileName = szTime;
        fullPath = LogFileDir + "/" + fileName + ".log";
        strLink = LogFileDir + "/" + PrefixName + ".log";
        unlink(strLink.data());
        symlink(fullPath.data(), strLink.data());
        m_LogFile = fopen(fullPath.data(), "a");
        if(NULL == m_LogFile)
        {
            return false;
        }
        m_sPrefixName = PrefixName;
        m_sLogfileDir = LogFileDir;
        setvbuf(m_LogFile,NULL,_IONBF,0);
    }
    return true;
}

void CLogThread::WriteFile(TMsg* pMsg)
{
    tm ptm = {0};
    time_t now = time(NULL);
    localtime_r((const time_t *) &now,&ptm);
    if ((ptm.tm_mday > m_CurrentTime.tm_mday) || (ptm.tm_mon > m_CurrentTime.tm_mon)
        || (ptm.tm_year > m_CurrentTime.tm_year))
    {
        NewDayLogFile();
    }

    TLogReq* req = (TLogReq*)pMsg;

    if (NULL != m_LogFile)
    {
        fprintf(m_LogFile, "[%04d-%02d-%02d][%02d:%02d:%02d]%s\n",
        (1900 + ptm.tm_year),(1 + ptm.tm_mon), ptm.tm_mday, ptm.tm_hour, ptm.tm_min,ptm.tm_sec,req->m_LogContent.data());

        fflush(m_LogFile);
    }
}

void CLogThread::NewDayLogFile()
{
    if (m_LogFile != NULL)
    {
        fclose(m_LogFile);
        m_LogFile = NULL;
    }
    OpenFile(m_sPrefixName, m_sLogfileDir);
}

