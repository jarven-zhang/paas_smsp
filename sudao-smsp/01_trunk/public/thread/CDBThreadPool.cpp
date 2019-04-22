#include "CDBThreadPool.h"
#include "ssl/md5.h"
#include "Fmt.h"
#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>


CDBThreadPool::CDBThreadPool(const char *name, UInt32 uThreadNum):
    CThread(name)
{
    m_uThreadNum = uThreadNum;
    m_iIndex = 0;
    pthread_mutex_init(&m_mutex, NULL);

}

CDBThreadPool::~CDBThreadPool()
{
    pthread_mutex_destroy(&m_mutex);
}

bool CDBThreadPool::Init(cs_t dbhost, unsigned int dbport, cs_t dbuser, cs_t dbpwd, cs_t dbname)
{
    if (m_uThreadNum <= 0 || m_uThreadNum > DBTHREAD_MAXNUM)
    {
        LogErrorP("dbThreadPool init failed, m_uThreadNum[%u] error.", m_uThreadNum);
        return false;
    }

    for (UInt32 i = 0; i < m_uThreadNum; i++)
    {
        CDBThread *pThread = new CDBThread(Fmt<32>("DbThread%u", i).data());
        INIT_CHK_NULL_RET_FALSE(pThread);
        INIT_CHK_FUNC_RET_FALSE(pThread->Init(dbhost, dbport, dbuser, dbpwd, dbname));
        INIT_CHK_FUNC_RET_FALSE(pThread->CreateThread());

        m_pThreadNode[i] = pThread;
    }

    return true;
}

int CDBThreadPool::GetMsgSize()
{
    pthread_mutex_lock(&m_mutex);
    int iMsgNum = 0;
    for(UInt32 i = 0; i < m_uThreadNum; i++)
    {
        iMsgNum = iMsgNum + m_pThreadNode[i]->GetMsgSize();
    }
    pthread_mutex_unlock(&m_mutex);
    return iMsgNum;
}

UInt32 CDBThreadPool::GetMsgCount()
{
    UInt32 iCount = 0;
    pthread_mutex_lock(&m_mutex);
    for(UInt32 i = 0; i < m_uThreadNum; i++)
    {
        iCount = iCount + m_pThreadNode[i]->GetMsgCount();
    }
    pthread_mutex_unlock(&m_mutex);
    return iCount;
}

void CDBThreadPool::SetMsgCount(int count)
{
    pthread_mutex_lock(&m_mutex);
    for(UInt32 i = 0; i < m_uThreadNum; i++)
    {
        m_pThreadNode[i]->SetMsgCount(count);
    }
    pthread_mutex_unlock(&m_mutex);
}

MYSQL *CDBThreadPool::CDBGetConn()
{
    return m_pThreadNode[0]->CDBGetConn();
}

bool CDBThreadPool::PostMsg(TMsg *pMsg)
{
    UInt32 index = 0;
    TDBQueryReq *pReq = (TDBQueryReq *)pMsg;
    if(pReq->m_strKey.empty())
    {
        ////chose a aviliable link
        bool bChoseDBLinkSUC = false;
        UInt32 round = m_uThreadNum;
        while(round--)
        {
            if(m_iIndex >= m_uThreadNum)
            {
                m_iIndex = 0;
            }

            index = m_iIndex;
            m_iIndex++;

            if(m_pThreadNode[index]->Available())
            {
                bChoseDBLinkSUC = true;
                break;
            }
        }

        ////maybe all link is false
        if(bChoseDBLinkSUC == false)
        {
            LogError("errror! all dblink is not  aviliable.sql[%s]", pReq->m_SQL.data());
            delete pMsg;
            return false;
        }

    }
    else
    {
        //get md5
        string strMd5;
        unsigned char md5[16] = { 0 };
        MD5((const unsigned char *) pReq->m_strKey.c_str(), pReq->m_strKey.length(), md5);
        std::string HEX_CHARS = "0123456789abcdef";
        for (int i = 0; i < 16; i++)
        {
            strMd5.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
            strMd5.append(1, HEX_CHARS.at(md5[i] & 0x0F));
        }

        //get md5 head 4
        strMd5 = strMd5.substr(0, 4);
        UInt64 uKey = strtol(strMd5.data(), NULL, 16);
        if(uKey > 0xffff)
        {
            delete pMsg;
            return false;
        }

        //get threadIndex
        index = ((uKey - 1) * m_uThreadNum / 0xffff);   //(ukey+1) /(0xffff/m_uThreadNum) = nodeNumber;     //if m_uThreadNum=10; index[0,9]
        if(index >= m_uThreadNum)
        {
            index = m_uThreadNum - 1;
        }

        //check db_node is aviliable,  if not aviliable then return false
        if(!m_pThreadNode[index]->Available())
        {
            LogError("errror db_node[%d] is not  aviliable. sql[%s]", index, pReq->m_SQL.data());
            delete pMsg;
            return false;
        }

    }

    //get aviliable_index suc
    LogDebug("index=%d, pReq->m_strKey[%s]", index, pReq->m_strKey.data());
    m_pThreadNode[index]->PostMsg(pMsg);
    return true;
}

void CDBThreadPool::HandleMsg(TMsg *pMsg)
{
    LogDebug("no reason to go here");
}

CDBThread *CDBThreadPool::getThread(UInt32 uiIdx) const
{
    if ((0 > uiIdx) || (DBTHREAD_MAXNUM <= uiIdx))
    {
        return NULL;
    }

    CHK_NULL_RETURN_NULL(m_pThreadNode);
    return m_pThreadNode[uiIdx];
}


