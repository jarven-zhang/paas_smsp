#include "CSqlFileThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include "CLogThread.h"


CSqlFileThread::CSqlFileThread(const char *name)
 : CThread(name), m_pSqlFile(NULL)
{
}

CSqlFileThread::~CSqlFileThread()
{
}


bool CSqlFileThread::Init(const std::string PrefixName, std::string fileDir)
{
    if(false == OpenFile(PrefixName, fileDir))
    {
        return false;
    }
    if(false == CThread::Init())
    {
        return false;
    }
    return true;
}


void CSqlFileThread::HandleMsg(TMsg* pMsg)
{
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
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
        default:
        {
            break;
        }
    }
}

bool CSqlFileThread::OpenFile(const std::string PrefixName, const std::string fileDir)
{
    if (NULL == m_pSqlFile)
    {
        tm ptm = {0};
        time_t now = time(NULL);
        std::string fileName;
        std::string fullPath;
        localtime_r((const time_t *) &now,&ptm);
        memcpy(&m_CurrentTime, &ptm, sizeof(tm));
        char szTime[60] = { 0 };
        snprintf(szTime, 60, "%s_%04d%02d%02d_%02d", PrefixName.c_str(), (1900 + ptm.tm_year),
                (1 + ptm.tm_mon), ptm.tm_mday, ptm.tm_hour);
        fileName = szTime;
        fullPath = fileDir + "/" + fileName + ".sql";
        m_pSqlFile = fopen(fullPath.c_str(), "a");
        if(NULL == m_pSqlFile)
        {
            return false;
        }
        m_strPrefixName = PrefixName;
        m_strSqlfileDir = fileDir;
        setvbuf(m_pSqlFile, NULL, _IONBF, 0);
    }
    return true;
}

void CSqlFileThread::WriteFile(TMsg* pMsg)
{
    tm ptm = {0};
    time_t now = time(NULL);
    localtime_r((const time_t *) &now,&ptm);
    if ((ptm.tm_hour > m_CurrentTime.tm_hour) || (ptm.tm_mday > m_CurrentTime.tm_mday)
		|| (ptm.tm_mon > m_CurrentTime.tm_mon) || (ptm.tm_year > m_CurrentTime.tm_year))
    {
        NewHourFile();
    }

    TSqlFileReq* req = (TSqlFileReq*)pMsg;

    if (NULL != m_pSqlFile)
    {
        fprintf(m_pSqlFile, "%s\n", req->m_strSqlContent.c_str());

        fflush(m_pSqlFile);
    }
}

void CSqlFileThread::NewHourFile()
{
    if (m_pSqlFile != NULL)
    {
        fclose(m_pSqlFile);
        m_pSqlFile = NULL;
    }
    OpenFile(m_strPrefixName, m_strSqlfileDir);
}

