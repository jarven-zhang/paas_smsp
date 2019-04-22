#include "CChannelTxThreadPool.h"
#include "CHTTPSendThread.h"
#include "main.h"
#include "UrlCode.h"
#include "Comm.h"
#include "alarm.h"
#include "base64.h"
#include "HttpsSender.h"
#include "json/json.h"
#include "SmsAccount.h"
#include "CSmsCluster.h"
#include "CRuleLoadThread.h"
#include "CChannelTxFlowCtr.h"
#include "Channel.h"
#include "HttpParams.h"


extern CSMSCluster*     g_CClusterThread;

#define SESSION_FREE( session ) SAFE_DELETE(session)

#define SS_TIME_WINDOW_SECS  2


channelTxSessions::~channelTxSessions()
{
    if (NULL != m_context)
    {
        delete m_context;
        m_context = NULL;
    }

    if (NULL != m_pWheelTime)
    {
        delete m_pWheelTime;
        m_pWheelTime = NULL;
    }

    if( NULL != m_reqMsg )
    {
        if(this->m_bDirectProt)
        {
            delete (smsDirectInfo *)m_reqMsg;
        }
        else
        {
            delete (SmsHttpInfo   *)m_reqMsg;
        }
        m_reqMsg = NULL;
    }

};


bool CChannelThreadPool::Init( UInt32 maxThreadCount )
{
    m_pSnMng = new SnManager();

    if (NULL == m_pSnMng )
    {
        LogError("m_pSnMng is NULL.");
        return false;
    }

    if ( false == CThread::Init())
    {
        LogError("CThread::Init is failed.");
        return false;
    }

    m_pVerify = new VerifySMS();
    if(m_pVerify == NULL)
    {
        printf("m_pVerify is NULL.");
        return false;
    }

    pthread_rwlock_init( &m_rwlock,  NULL );
    pthread_mutex_init( &m_statusMutex, NULL );

    m_mapChannelErrorCode.clear();
    g_pRuleLoadThread->getChannelErrorCode(m_mapChannelErrorCode);

    m_mapSystemErrorCode.clear();
    g_pRuleLoadThread->getSystemErrorCode(m_mapSystemErrorCode);

    m_mqConfigInfo.clear();
    g_pRuleLoadThread->getMqConfig(m_mqConfigInfo);

    m_componentRefMqInfo.clear();
    g_pRuleLoadThread->getComponentRefMq(m_componentRefMqInfo);

    m_accountMap.clear();
    g_pRuleLoadThread->getAccountMap(m_accountMap);

    m_ChannelMap.clear();
    g_pRuleLoadThread->getChannlelMap( m_ChannelMap );

    m_ChannelSgipMap.clear();
    g_pRuleLoadThread->getSgipChannlelMap( m_ChannelSgipMap );

    m_mapSignExtGw.clear();
    g_pRuleLoadThread->getClientIdAndAppend(m_mapSignExtGw);

    map<string,string> mapSysParam;
    g_pRuleLoadThread->getSysParamMap(mapSysParam);
    GetSysPara(mapSysParam, true);

    m_mapChanneThread.clear();
    m_mapSessions.clear();
    m_poolTxThreads.clear();
    m_poolTxDirectThreads.clear();

    m_mapChannelFlowCtr.clear();

    if( maxThreadCount == 0  )
    {
        maxThreadCount = CC_TX_CHANNEL_THREAD_NUM_DEF;
    }

    if( maxThreadCount > CC_TX_CHANNEL_THREAD_NUM_MAX )
    {
        maxThreadCount = CC_TX_CHANNEL_THREAD_NUM_MAX;
    }

    printf("\nCChannelThreadPool maxThreadCount[%u]\n", maxThreadCount);

    /*初始化http处理线程*/
    for( UInt32 i = 0; i < maxThreadCount; i++ )
    {
        CHTTPSendThread* pThread = new CHTTPSendThread("CHTTPSendThread");
        if( true == pThread->Init(i) && true == pThread->CreateThread() )
        {
            m_poolTxThreads.push_back(pThread);
        }
        else
        {
            printf("****** CreateHttpSendThread %u Fail ****\n", i);
            SAFE_DELETE( pThread );
            return false;
        }
    }

    LogNotice("****** Create %d HttpSendThread ******", m_poolTxThreads.size());

    /* 创建线程数量*/
    for( UInt32 i = 0; i < maxThreadCount /*&& i < uMaxLinkNum*/; i++ )
    {
        string direThreadName = "DirectThread_" + Comm::int2str(i);
        CDirectSendThread *pThread = new CDirectSendThread(direThreadName.data());
        if( true == pThread->Init(i) && true == pThread->CreateThread())
        {
            pThread->InitContinueLoginFailedValue(m_uContinueLoginFailedValue);
            pThread->InitChannelCloseWaitTime(m_uChannelCloseWaitTime);
            m_poolTxDirectThreads.push_back( pThread );
        }
        else
        {
            printf("****** CreateDirectSendThread %u Fail ****\n", i);
            SAFE_DELETE(pThread);
            return false;
        }
    }

    LogNotice("****** Create %d DirectSendThread ******", m_poolTxDirectThreads.size());

    /*初始化线程与通道的绑定关系*/
    for(channelMap_t::iterator itr = m_ChannelMap.begin(); itr != m_ChannelMap.end(); itr++ )
    {
        if( itr->second.httpmode <= HTTP_POST ){
            channelThreadsData_t threadData;
            threadData.m_uPoolIdx = 0;
            threadData.m_vecThreads = m_poolTxThreads;
            m_mapChanneThread[itr->first] = threadData ;

            PoolChannelFlowCtrNew(itr->first, itr->second.m_uSliderWindow, itr->second.m_uClusterType != 0  );
            LogDebug("m_mapChanneThread add Channel[%d] and init %d HttpSendThread", itr->first, m_poolTxThreads.size());
        }
        else
        {
            channelThreadsData_t threadData;
            UInt32               maxConnect = 1;

            if( itr->second.m_uLinkNodenum > m_poolTxDirectThreads.size())
            {
                maxConnect = m_poolTxDirectThreads.size();
                LogWarn("Channel[%d] max node num[%d]", itr->first, maxConnect);
            }
            else
            {
                maxConnect = itr->second.m_uLinkNodenum;
            }

            InitDirectChannelThreads(maxConnect, itr->first);

            if(false == InitChannelThreadInfo(itr->first))
            {
                printf("InitChannelThreadInfo Error!");
                return false;
            }

            LogDebug("m_mapChanneThread add Channel[%d] and init %d DirectSendThread", itr->first, maxConnect);
        }
    }

    m_pFlowWheelTime = SetTimer( CC_TX_CHANNEL_FLOW_RESET_TIMER_ID, "", CC_TX_CHANNEL_WINDOWSIZE_CHECK_INTERVAL*2 );

    m_timerSynSlideWnd = SetTimer( CC_TX_CHANNEL_SYSN_SLIDEWND_TIMER_ID, "", CC_TX_CHANNEL_SYSN_SLIDEWND_TIME*2 );

    return true;
}

void CChannelThreadPool::GetSysPara(const std::map<std::string, std::string>& mapSysPara, bool bInit)
{
    string strSysPara;
    std::map<std::string, std::string>::const_iterator iter;

    do
    {
        int nTmp = 0;
        strSysPara = "SIGN_MO_PORT_EXPIRE";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;
        size_t pos = strTmp.find(";");
        if(pos == std::string::npos)
        {
            LogError("system parameter(%s) is not have ';' value(%s)", strSysPara.c_str(), strTmp.c_str());
            break;
        }
        nTmp = atoi(strTmp.substr(0, pos).data()) * 60;
        if (nTmp <= 0)
        {
            LogError("Invalid system parameter(%s) value(%s).", strSysPara.c_str(), strTmp.c_str());
            break;
        }

        m_uSignMoPortExpire = nTmp;
    }
    while (0);
    LogNotice("System parameter(%s) value(%s).", strSysPara.c_str(), iter->second.data());

    do
    {
        int nTmp = 0;
        strSysPara = "SEND_AUDIT_INTERCEPT";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;
        size_t pos = strTmp.find(";");
        if(pos == std::string::npos)
        {
            LogError("system parameter(%s) is not have ';' value(%s)", strSysPara.c_str(), strTmp.c_str());
            break;
        }
        nTmp = atoi(strTmp.substr(0, pos).data());
        if (nTmp < 0  || nTmp > 1)
        {
            LogError("Invalid system parameter(%s) value(%s).", strSysPara.c_str(), strTmp.c_str());
            break;
        }

        m_uSendAuditInterceptSwitch = nTmp;
    }
    while (0);
    LogNotice("System parameter(%s) value(%s).", strSysPara.c_str(), iter->second.data());

    do
    {
        strSysPara = "CHANNEL_RES_FAIL";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;

        int nTmp = atoi(strTmp.c_str());
        if (0 > nTmp)
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(), strTmp.c_str(), nTmp);
            break;
        }

        m_uSendFailedValue = nTmp;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u).", strSysPara.c_str(), m_uSendFailedValue);

    do
    {
        strSysPara = "CHANNEL_LINK_FAIL";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;

        int nTmp = atoi(strTmp.c_str());
        if (0 > nTmp)
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(), strTmp.c_str(), nTmp);
            break;
        }

        m_uSubmitFailedValue = nTmp;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u).", strSysPara.c_str(), m_uSubmitFailedValue);

    do
    {
        strSysPara = "CONTINUE_LOGIN_FAILED_NUM";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;

        int nTmp = atoi(strTmp.c_str());
        if (0 > nTmp)
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(), strTmp.c_str(), nTmp);
            break;
        }

        if(!bInit)
        {
            CheckContinueLoginFailedValueChanged(m_uContinueLoginFailedValue, nTmp);
        }
        m_uContinueLoginFailedValue = nTmp;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u).", strSysPara.c_str(), m_uContinueLoginFailedValue);

    do
    {
        strSysPara = "SELECT_CHANNEL_NUM";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;

        int nTmp = atoi(strTmp.c_str());
        if (0 > nTmp)
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(), strTmp.c_str(), nTmp);
            break;
        }

        m_uMaxSendTimes = nTmp;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u).", strSysPara.c_str(), m_uMaxSendTimes);

    do
    {
        strSysPara = "FAILED_RESEND_CFG";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        std::string strTmp = iter->second;
        if (strTmp.empty())
        {
            LogError("system parameter(%s) is empty, pls reconfig.", strSysPara.c_str());
            break;
        }

        int index = -1;
        if(-1 == (index = strTmp.find("|")))
        {
            LogError("system parameter(%s) cfg(%s)format error",strSysPara.c_str(),strTmp.c_str());
            break;
        }
        m_bResendSysSwitch = (1 == atoi(strTmp.substr(0,index).c_str()));
        string strSysCfg = strTmp.substr(index+1);
        vector<std::string> vecCfg;
        Comm::split(strSysCfg, ";", vecCfg);
        //m_FailedResendSysCfgMap.clear();
        for(UInt32 i = 0; i < vecCfg.size(); i++)
        {

            vector<string> vecData;
            Comm::split(vecCfg[i], ",", vecData);

            if(3 != vecData.size())
                continue;

            FailedResendSysCfg cfginfo;
            Int32 smstype = atoi(vecData[0].data());
            Int32 MaxTimes = atoi(vecData[1].data());
            Int32 MaxDuration = atoi(vecData[2].data());
            if(smstype < 0 ||  MaxTimes < 0 || MaxTimes > 50 || MaxDuration < 1 || MaxDuration > 86400)
            {
                LogWarn("FAILED_RESEND_CFG Invalid value: smstype[%d],MaxTimes[%d],MaxDuration[%d]",smstype,MaxTimes,MaxDuration);
                continue;
            }
            cfginfo.m_uResendMaxTimes = MaxTimes;
            cfginfo.m_uResendMaxDuration = MaxDuration;
            m_FailedResendSysCfgMap[(UInt32)smstype] = cfginfo;
            LogNotice("smsytpe[%d],ResendMaxTimes[%d],ResendMaxDuration[%d]",smstype,MaxTimes,MaxDuration);
        }
    }
    while (0);
    LogNotice("System parameter(%s) value m_bResendSysSwitch[%d],m_FailedResendSysCfgMap size[%d]",
        strSysPara.c_str(), m_bResendSysSwitch ? 1:0 ,m_FailedResendSysCfgMap.size());


    do
    {
        strSysPara = "CHANNEL_CLOSE_WAIT_RESP_TIME";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;

        int nTmp = atoi(strTmp.c_str());
        if (0 > nTmp)
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(), strTmp.c_str(), nTmp);
            break;
        }
        if(!bInit)
        {
            CheckChannelCloseWaitTimeChanged(m_uChannelCloseWaitTime, nTmp);
        }
        m_uChannelCloseWaitTime = nTmp;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u).", strSysPara.c_str(), m_uChannelCloseWaitTime);
}

bool CChannelThreadPool::InitChannelThreadInfo(int ChannelId)
{
    channelMap_t::iterator itChMap = m_ChannelMap.find(ChannelId);
    if(itChMap == m_ChannelMap.end())
    {
        LogError("Channel[%d] is not found in m_ChannelMap", ChannelId);
        return false;
    }

    channeThreadMap_t::iterator itChThdMap = m_mapChanneThread.find(ChannelId);
    if(itChThdMap == m_mapChanneThread.end())
    {
        LogError("Channel[%d] is not found in m_mapChanneThread", ChannelId);
        return false;
    }

    ChannelLoginStateValue_t StateValue;
    for(channeThreads_t::iterator itor = itChThdMap->second.m_vecThreads.begin(); itor != itChThdMap->second.m_vecThreads.end(); itor++)
    {
        TDirectChannelUpdateMsg* addMsg = new TDirectChannelUpdateMsg();
        addMsg->m_Channel  = itChMap->second;
        addMsg->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_ADD_REQ;
        static_cast<CDirectSendThread*>(*itor)->PostMsg(addMsg);
        StateValue[static_cast<CDirectSendThread*>(*itor)->m_uThreadId] = CHANNEL_UN_LOGIN;
    }

    //登录状态记录
    m_mapChannelLoginState.insert(make_pair(ChannelId, StateValue));

    PoolChannelFlowCtrNew( ChannelId, itChMap->second.m_uSliderWindow * itChMap->second.m_uLinkNodenum );

    return true;
}

void CChannelThreadPool::PrintLoginState(int ChannelId)
{
    ChannelLoginState_t::iterator iter = m_mapChannelLoginState.find(ChannelId);
    if(iter != m_mapChannelLoginState.end())
    {
        for(ChannelLoginStateValue_t::iterator itor = iter->second.begin(); itor != iter->second.end(); itor++)
        {
            LogWarn("------Channel[%d] Node[%d] State is %d------", ChannelId, itor->first, itor->second);
        }
    }
}

//获取实际可用（登陆成功）的节点数
int CChannelThreadPool::GetWorkChannelNodeNum(int ChannelId)
{
    int workNode = 0;
    ChannelLoginState_t::iterator iter = m_mapChannelLoginState.find(ChannelId);
    if(iter != m_mapChannelLoginState.end())
    {
        for(ChannelLoginStateValue_t::iterator it = iter->second.begin(); it != iter->second.end(); it++)
        {
            if(it->second == CHANNEL_LOGIN_SUCCESS)
            {
                workNode++;
            }
        }
    }

    return workNode;
}

bool CChannelThreadPool::AdaptChannelNodeNumChanged(int ChannelId, int newNodes, int oldWnd, int newWnd)
{
    if(newNodes < 1)
    {
        LogWarn("Channel[%d] link nodenum[%d] less than 1, forced change it to 1.", ChannelId, newNodes);
        newNodes = 1;
    }

    channelMap_t::iterator itChMap = m_ChannelMap.find(ChannelId);
    if(itChMap == m_ChannelMap.end())
    {
        LogError("Channel[%d] is not found in m_ChannelMap", ChannelId);
        return false;
    }

    if(itChMap->second.httpmode <= HTTP_POST)
    {
        LogNotice("Channel[%d] is http protocol, ignore it", ChannelId);
        return false;
    }

    channeThreadMap_t::iterator itChThdMap = m_mapChanneThread.find(ChannelId);
    if(itChThdMap == m_mapChanneThread.end())
    {
        LogError("Channel[%d] is not found in m_mapChanneThread", ChannelId);
        return false;
    }

    //老的结点数
    int oldNodes = itChThdMap->second.m_vecThreads.size();

    //老的实际可用的节点数
    int oldWorkNodes = GetWorkChannelNodeNum(ChannelId);

    LogDebug("Channel[%d] oldNodesNum[%d:%d] newNodesNum[%d]", ChannelId, oldWorkNodes, oldNodes, newNodes);

    if(oldNodes > newNodes)//删
    {
        int i = oldNodes - newNodes;
        int wnd = 0;
        int state;

        ChannelLoginState_t::iterator iterLogin = m_mapChannelLoginState.find(ChannelId);
        if(iterLogin == m_mapChannelLoginState.end())
        {
            LogError("Channel[%d] not found in m_mapChannelLoginState", ChannelId);
            return false;
        }

        if(oldWnd != newWnd)
        {
            PoolChannelFlowCtrReset(ChannelId, newWnd * oldWorkNodes);
        }

        //有删掉本来就不可用的结点
        for(channeThreads_t::iterator iterThread = itChThdMap->second.m_vecThreads.begin(); iterThread != itChThdMap->second.m_vecThreads.end() && i > 0; )
        {
            state = static_cast<CDirectSendThread*>(*iterThread)->GetFlowState(ChannelId, wnd);

            if(state != CHANNEL_FLOW_STATE_NORMAL)
            {
                //删除登陆状态关系
                ChannelLoginStateValue_t::iterator iterLoginVal = iterLogin->second.find(static_cast<CDirectSendThread*>(*iterThread)->m_uThreadId);
                LogDebug("Channel[%d] delete bad node %d", ChannelId, static_cast<CDirectSendThread*>(*iterThread)->m_uThreadId);
                if(iterLoginVal != iterLogin->second.end())
                {
                    iterLogin->second.erase(iterLoginVal);
                }

                TDirectChannelUpdateMsg* delMsg = new TDirectChannelUpdateMsg();
                delMsg->m_iChannelId = ChannelId;
                delMsg->m_Channel = itChMap->second;
                delMsg->m_lDelTime = time(NULL);
                delMsg->m_iMsgType   = MSGTYPE_RULELOAD_CHANNEL_DEL_REQ;
                static_cast<CDirectSendThread*>(*iterThread)->PostMsg(delMsg);

                i--;
                iterThread = itChThdMap->second.m_vecThreads.erase(iterThread);
            }
            else
            {
                iterThread++;
            }
        }

        //不可用的结点可能不够删，这里在删掉一些可用的结点
        for(channeThreads_t::iterator iterThread = itChThdMap->second.m_vecThreads.begin(); iterThread != itChThdMap->second.m_vecThreads.end() && i > 0; )
        {
            //删除登陆状态关系
            ChannelLoginStateValue_t::iterator iterLoginVal = iterLogin->second.find(static_cast<CDirectSendThread*>(*iterThread)->m_uThreadId);
            LogDebug("Channel[%d] delete good node %d", ChannelId, static_cast<CDirectSendThread*>(*iterThread)->m_uThreadId);
            if(iterLoginVal != iterLogin->second.end())
            {
                iterLogin->second.erase(iterLoginVal);
            }

            TDirectChannelUpdateMsg* delMsg = new TDirectChannelUpdateMsg();
            delMsg->m_iChannelId = ChannelId;
            delMsg->m_Channel = itChMap->second;
            delMsg->m_lDelTime = time(NULL);
            delMsg->m_iMsgType   = MSGTYPE_RULELOAD_CHANNEL_DEL_REQ;
            static_cast<CDirectSendThread*>(*iterThread)->PostMsg(delMsg);

            i--;
            iterThread = itChThdMap->second.m_vecThreads.erase(iterThread);
        }

        itChThdMap->second.m_uPoolIdx = 0;
    }
    else if(oldNodes < newNodes)
    {
        //增加结点数了
        int needNewNodes = newNodes - oldNodes;
        if(newNodes > (int)m_poolTxDirectThreads.size())
        {
            needNewNodes = (int)m_poolTxDirectThreads.size() - oldNodes;
            LogWarn("Channel[%d] has %d thread already, it can new %d thread at most!", ChannelId, oldNodes, needNewNodes);
        }

        if(needNewNodes <= 0)
        {
            LogWarn("Channel[%d] needNewNodes is 0", ChannelId);
            return true;
        }

        if(oldWnd != newWnd)
        {
            //新节点登陆成功会上报登陆状态，新增滑窗
            PoolChannelFlowCtrReset(ChannelId, newWnd * oldWorkNodes);
        }

        ChannelLoginState_t::iterator iterLogin = m_mapChannelLoginState.find(ChannelId);
        if(iterLogin == m_mapChannelLoginState.end())
        {
            LogError("Channel[%d] not found in m_mapChannelLoginState", ChannelId);
            return false;
        }

        for(channeThreads_t::iterator it = m_poolTxDirectThreads.begin(); it != m_poolTxDirectThreads.end() && needNewNodes > 0; it++)
        {
            UInt32 tid = static_cast<CDirectSendThread*>(*it)->m_uThreadId;
            UInt32 tmptid = 0;
            bool   bUse = true;         //标识通道能否用当前线程
            string strPoolThreads = " ";

            LogDebug("Channel[%d] pool try to increase thread[%u]", ChannelId, tid);

            //检查一下这个通道是否已经挂在该线程上
            for(channeThreads_t::iterator itor = itChThdMap->second.m_vecThreads.begin(); itor != itChThdMap->second.m_vecThreads.end(); itor++)
            {
                tmptid = static_cast<CDirectSendThread*>(*itor)->m_uThreadId;
                if(tmptid == tid)
                {
                    bUse = false;
                    break;
                }
                strPoolThreads = strPoolThreads + Comm::int2str(tmptid) + " ";
            }

            if(bUse == true)
            {
                itChThdMap->second.m_vecThreads.push_back(*it);
                TDirectChannelUpdateMsg* addMsg = new TDirectChannelUpdateMsg();
                addMsg->m_Channel  = itChMap->second;
                addMsg->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_ADD_REQ;
                static_cast<CDirectSendThread*>(*it)->PostMsg(addMsg);

                iterLogin->second.insert(make_pair((int)tid, (int)CHANNEL_UN_LOGIN));

                needNewNodes--;
                strPoolThreads = " " + Comm::int2str(tid) + strPoolThreads;
                LogNotice("Channel[%d] pool increase thread[%u], pool has thread[%s]", ChannelId, tid, strPoolThreads.data());
            }
        }

        itChThdMap->second.m_uPoolIdx = 0;
    }
    else
    {
        //不增不减
        LogWarn("Channel[%d], Should not come here!", ChannelId);
    }

    //PrintLoginState(ChannelId);
    return true;
}

void CChannelThreadPool::InitDirectChannelThreads(UInt32 maxConnect, int channelId)
{
    static unsigned int  id = 0;
    channelThreadsData_t chanThreadData;
    string               desc = " ";

    if(maxConnect < 1)
    {
        LogWarn("Channel[%d] link nodenum[%d] less than 1, forced change it to 1.", channelId, maxConnect);
        maxConnect = 1;
    }

    for(UInt32 i = 0; i < maxConnect; i++)
    {
        if(id >=  m_poolTxDirectThreads.size())
        {
            id = 0;
        }
        desc = desc + Comm::int2str(id) + " ";
        chanThreadData.m_vecThreads.push_back(m_poolTxDirectThreads[id++]);
    }
    chanThreadData.m_uPoolIdx = 0;

    m_mapChanneThread[channelId] = chanThreadData;
    LogNotice("Channel[%d] NodeNum[%u], the ThreadPool has Thread[%s]", channelId, maxConnect, desc.data());
}

/* 增加队列区分处理逻辑*/
bool CChannelThreadPool::smsTxPostMsg( TMsg *pMsg )
{
    /*状态报告优先处理队列*/
    if( pMsg->m_iMsgType == MSGTYPE_CHANNEL_SEND_STATUS_REPORT
        || pMsg->m_iMsgType == MSGTYPE_DIRECT_TO_DELIVERYREPORT_REQ)
    {
        pthread_mutex_lock(&m_statusMutex);
        m_msgStatusQueue.AddMsg(pMsg);
        pthread_mutex_unlock(&m_statusMutex);
    }else{
        PostMsg( pMsg );
    }

    return true;
}


bool CChannelThreadPool::smsTxProcessStatusQueue()
{
    bool  ret = false  ;
    TMsg* pMsg = NULL;

    for( UInt32 i = 0; i < 10; i++ )
    {
        pthread_mutex_lock(&m_statusMutex);
        pMsg = m_msgStatusQueue.GetMsg();
        pthread_mutex_unlock(&m_statusMutex);

        if( pMsg != NULL ){
            HandleMsg( pMsg );
            delete pMsg;
            ret = true;
        }else{
            break;
        }
    }
    return ret;

}


void CChannelThreadPool::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while(true)
    {
        m_pTimerMng->Click();

        /* 响应数据处理*/
        bool ret = smsTxProcessStatusQueue();

        pthread_mutex_lock(&m_mutex);
        TMsg* pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if( pMsg == NULL && ret == false)
        {
            usleep(g_uSecSleep);
        }
        else if (NULL != pMsg)
        {
            HandleMsg( pMsg );
            delete pMsg;
        }
    }
}

void CChannelThreadPool::CheckChannelCloseWaitTimeChanged(UInt32 oldValue, UInt32 newValue)
{
    if(oldValue == newValue)
    {
        return;
    }

    for(channeThreads_t::iterator iter = m_poolTxDirectThreads.begin(); iter != m_poolTxDirectThreads.end(); iter++)
    {
        TChannelCloseWaitTimeMsg *pMsg = new TChannelCloseWaitTimeMsg();
        pMsg->m_uValue = newValue;
        pMsg->m_iMsgType = MSGTYPE_CHANNEL_CLOSE_WAIT_TIME_UPDATE;
        static_cast<CDirectSendThread*>(*iter)->PostMsg(pMsg);
    }
}

void CChannelThreadPool::CheckContinueLoginFailedValueChanged(UInt32 oldValue, UInt32 newValue)
{
    if(oldValue == newValue)
    {
        return;
    }

    for(channeThreads_t::iterator iter = m_poolTxDirectThreads.begin(); iter != m_poolTxDirectThreads.end(); iter++)
    {
        TContinueLoginFailedValueMsg *pMsg = new TContinueLoginFailedValueMsg();
        pMsg->m_uValue = newValue;
        pMsg->m_iMsgType = MSGTYPE_CONTINUE_LOGIN_FAIL_VALUE_UPDATE;
        static_cast<CDirectSendThread*>(*iter)->PostMsg(pMsg);
    }
}

void CChannelThreadPool::HandleMsg( TMsg* pMsg )
{
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    switch( pMsg->m_iMsgType )
    {
        /* 用户账号变更*/
        case MSGTYPE_RULELOAD_USERGW_UPDATE_REQ:
        {
            TUpdateAccountReq* pAccount = (TUpdateAccountReq*)pMsg;
            m_accountMap.clear();
            m_accountMap = pAccount->m_accountMap;
            LogNotice("update t_sms_account size:%d.",m_accountMap.size());
            break;
        }
        case MSGTYPE_TIMEOUT_RESET_SLIDE_WINDOW:
        {
            HandleTimeOutResetSlideWindowMsg(pMsg);
            break;
        }
        /*http请求发送*/
        case MSGTYPE_DISPATCH_TO_HTTPSEND_REQ:
        {

            smsTxHttpChannelReq( pMsg );
            break;
        }
        //直连协议发送
        case MSGTYPE_DISPATCH_TO_DIRECTSEND_REQ:
        {
            smsTxDirectChannelReq( pMsg );
            break;
        }

        case MSGTYPE_CHANNEL_SEND_REQ:
        {
            processChannelReq( pMsg );
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_ERROR_CODE_UPDATE_REQ:
        {
            pthread_rwlock_wrlock( &m_rwlock );
            TUpdateChannelErrorCodeReq* pHttp = (TUpdateChannelErrorCodeReq*)pMsg;
            m_mapChannelErrorCode.clear();
            m_mapChannelErrorCode = pHttp->m_mapChannelErrorCode;
            pthread_rwlock_unlock( &m_rwlock );

            LogNotice("update t_sms_channel_error_desc size:%d.",m_mapChannelErrorCode.size());
            break;
        }
        /*系统错误码更新*/
        case MSGTYPE_RULELOAD_SYSTEM_ERROR_DESC_EEQ:
        {
            pthread_rwlock_wrlock( &m_rwlock );
            TUpdateSystemErrorCodeReq* pHttp = (TUpdateSystemErrorCodeReq*)pMsg;
            m_mapSystemErrorCode.clear();
            m_mapSystemErrorCode = pHttp->m_mapSystemErrorCode;
            pthread_rwlock_unlock( &m_rwlock );
            LogNotice("update t_sms_system_error_desc size:%d.",m_mapSystemErrorCode.size());
            break;
        }
        case MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ:
        {
            TUpdateMqConfigReq* pMqCon = (TUpdateMqConfigReq*)pMsg;
            m_mqConfigInfo.clear();
            m_mqConfigInfo = pMqCon->m_mapMqConfig;
            LogNotice("update t_sms_mq_configure size:%d.",m_mqConfigInfo.size());
            break;
        }
        case MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ:
        {
            TUpdateComponentRefMqConfigReq* pMqCom = (TUpdateComponentRefMqConfigReq*)pMsg;
            m_componentRefMqInfo.clear();
            m_componentRefMqInfo = pMqCom->m_componentRefMqInfo;
            LogNotice("update t_sms_component_ref_mq size:%d.",m_componentRefMqInfo.size());
            break;
        }
        case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
        {
            TUpdateSysParamRuleReq* pSysParamReq = (TUpdateSysParamRuleReq*)pMsg;
            if (NULL == pSysParamReq)
            {
                break;
            }

            GetSysPara(pSysParamReq->m_SysParamMap);
            break;
        }
        case MSGTYPE_CHANNEL_SEND_STATUS_REPORT:
        {
            smsTxChannelSendStatusReport( pMsg );
            break;
        }
        /*通道状态变更*/
        case MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ:
        {
            smsTxUpdateChannel( pMsg );
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_SGIP_UPDATE_REQ:
        {
            smsTxUpdateSgipChannel( pMsg );
            break;
        }
        case MSGTYPE_RULELOAD_SIGNEXTNOGW_UPDATE_REQ:
        {
            TUpdateSignextnoGwReq* pSign = (TUpdateSignextnoGwReq*)pMsg;
            LogNotice("RuleUpdate signextno_gw update,map size[%d].",pSign->m_SignextnoGwMap.size());
            m_mapSignExtGw.clear();
            m_mapSignExtGw = pSign->m_SignextnoGwMap;
            break;
        }
        /* 通道状态报告*/
        case MSGTYPE_DIRECT_TO_DELIVERYREPORT_REQ:
        {
            HandleDirectToDeliveryReqMsg(pMsg);//处理直连协议状态报告
            break;
        }

        /*通道上行处理*/
        case MSGTYPE_TO_DIRECTMO_REQ:
        {

        }

        //通道登录状态改变
        case MSGTYPE_RULELOAD_CHANNEL_LOGIN_STATE_REQ:
        {
            HandleChannelLoginStateMsg(pMsg);
            break;
        }

        case MSGTYPE_TIMEOUT:
        {
            HandleTimeOut(pMsg);
            break;
        }
        case MSGTYPE_REDIS_RESP:
        {
            HandleRedisRespMsg(pMsg);
            break;
        }
        default:
        {
            LogWarn("unknown msgid[%x]!", pMsg->m_iMsgType);
            break;
        }
    }
}

void CChannelThreadPool::GetFinalChannelLoginState(ChannelLoginStateValue_t mapLoginValue, int& finalState, char* desc)
{
    int success = 0, fail = 0, unlogin = 0;
    for(ChannelLoginStateValue_t::iterator iter = mapLoginValue.begin(); iter != mapLoginValue.end(); iter++)
    {
        if(iter->second == CHANNEL_UN_LOGIN)
        {
            unlogin++;
        }
        else if(iter->second == CHANNEL_LOGIN_SUCCESS)
        {
            success++;
        }
        else
        {
            fail++;
        }
    }

    if(success > 0)
    {
        finalState = CHANNEL_LOGIN_SUCCESS;
    }
    else
    {
        finalState = CHANNEL_LOGIN_FAIL;
    }

    sprintf(desc, "%d success, %d fail, %d unlogin", success, fail, unlogin);
}

/* 连接断开后需要清除对应的线程*/
void CChannelThreadPool::HandleChannelLoginStateMsg(TMsg* pMsg)
{
    TUpdateChannelLoginStateReq* pLoginMsg = (TUpdateChannelLoginStateReq*)pMsg;

    int finalState  = 0;
    char   strDesc[256] = {0};

    //时间
    struct tm timenow = {0};
    time_t now = time(NULL);
    localtime_r(&now,&timenow);
    char dateTime[64] = {0};

    strftime(dateTime,sizeof(dateTime),"%Y%m%d%H%M%S",&timenow);

    ChannelLoginState_t::iterator itor = m_mapChannelLoginState.find(pLoginMsg->m_iChannelId);

    if(itor != m_mapChannelLoginState.end())
    {
        if(pLoginMsg->m_bDelAll)
        {
            LogNotice("Channel[%d] may changed protocol before, ignore this message", pLoginMsg->m_iChannelId);
            return;
        }

        ChannelLoginStateValue_t::iterator iter = itor->second.find(pLoginMsg->m_NodeId);

        if(iter != itor->second.end())
        {
            iter->second = pLoginMsg->m_uLoginStatus;
        }

        GetFinalChannelLoginState(itor->second, finalState, strDesc);

        //PrintLoginState(pLoginMsg->m_iChannelId);

        //更新滑窗
        PoolChannelFlowCtrModify(pLoginMsg->m_iChannelId, pLoginMsg->m_iWnd, GetWorkChannelNodeNum(pLoginMsg->m_iChannelId)*pLoginMsg->m_iMaxWnd);
    }
    else
    {
        if(!pLoginMsg->m_bDelAll)
        {
            LogWarn("Channel[%d] is not found in m_mapChannelLoginState, Size[%d]",
                pLoginMsg->m_iChannelId, m_mapChannelLoginState.size());
        }

        finalState = 2;
        strcpy(strDesc, "channel is close");
    }

    //更新数据库
    char sql[1024];
    snprintf(sql,1024,"INSERT INTO t_sms_channel_login_status (cid, login_status, login_desc, update_time) "
        "value('%d', '%u', '%s', '%s') ON DUPLICATE KEY UPDATE login_status='%d', login_desc='%s', update_time='%s'",
        pLoginMsg->m_iChannelId,
        finalState,
        strDesc,
        dateTime,
        finalState,
        strDesc,
        dateTime
    );

    LogNotice("Update Channel[%d:%d] Login State, sql[%s]", pLoginMsg->m_iChannelId, pLoginMsg->m_NodeId, sql);

    TDBQueryReq* pDBMsg = new TDBQueryReq();
    pDBMsg->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
    pDBMsg->m_SQL = sql;
    g_pDisPathDBThreadPool->PostMsg(pDBMsg);
}

void CChannelThreadPool::HandleDirectToDeliveryReqMsg( TMsg *pMsg )
{
    TDirectToDeliveryReportReqMsg* pRecvMsg = (TDirectToDeliveryReportReqMsg*)pMsg;

    TDirectToDeliveryReportReqMsg* pDeliveryMsg = new TDirectToDeliveryReportReqMsg();
    pDeliveryMsg->m_bSaveReport = pRecvMsg->m_bSaveReport;
    pDeliveryMsg->m_iStatus = pRecvMsg->m_iStatus;
    pDeliveryMsg->m_lReportTime = pRecvMsg->m_lReportTime;
    pDeliveryMsg->m_strChannelSmsId = pRecvMsg->m_strChannelSmsId;
    pDeliveryMsg->m_strReportStat = pRecvMsg->m_strErrorCode;
    pRecvMsg->m_strInnerErrorcode = pRecvMsg->m_strErrorCode;

    if ((UInt32)pRecvMsg->m_iStatus == SMS_STATUS_CONFIRM_FAIL)
    {
        string directType;
        if (YD_CMPP == pRecvMsg->m_iHttpMode || YD_CMPP3 == pRecvMsg->m_iHttpMode)
        {
            directType.assign("cmpp");
        }
        else if (DX_SMGP == pRecvMsg->m_iHttpMode)
        {
            directType.assign("smgp");
        }
        else if (GJ_SMPP == pRecvMsg->m_iHttpMode)
        {
            directType.assign("smpp");
        }
        else if (LT_SGIP == pRecvMsg->m_iHttpMode)
        {
            directType.assign("sgip");
        }
        else
        {
            LogError("m_uHttpmode[%d] error", pRecvMsg->m_iHttpMode);
        }
        smsTxDirectGetErrorCode(directType,pRecvMsg->m_uChannelId,string("2"),pRecvMsg->m_strDesc,pRecvMsg->m_strDesc,pRecvMsg->m_strErrorCode);
        smsTxDirectGetInnerError(directType,pRecvMsg->m_uChannelId,string("2"),pDeliveryMsg->m_strReportStat, pRecvMsg->m_strInnerErrorcode);

    }
    pDeliveryMsg->m_strDesc = pRecvMsg->m_strDesc;
    pDeliveryMsg->m_strErrorCode = pRecvMsg->m_strErrorCode;
        //remove 86
    if (m_pVerify->CheckPhoneFormat(pRecvMsg->m_strPhone) == false)
    {
        LogError("channel return phone[%s] format error", pRecvMsg->m_strPhone.data())
    }
    pDeliveryMsg->m_strPhone = pRecvMsg->m_strPhone;
    pDeliveryMsg->m_uChannelId = pRecvMsg->m_uChannelId;
    pDeliveryMsg->m_uChannelIdentify = pRecvMsg->m_uChannelIdentify;
    pDeliveryMsg->m_iMsgType = pRecvMsg->m_iMsgType;
    pDeliveryMsg->m_iSeq = pRecvMsg->m_iSeq;
    pDeliveryMsg->m_strSessionID = pRecvMsg->m_strSessionID;
    pDeliveryMsg->m_iHttpMode = pRecvMsg->m_iHttpMode;
    pDeliveryMsg->m_industrytype = GetChannelIndustrytype(pRecvMsg->m_uChannelId);
    pDeliveryMsg->m_strInnerErrorcode = pRecvMsg->m_strInnerErrorcode;

    g_pDeliveryReportThread->PostMsg(pDeliveryMsg);
}

UInt32 CChannelThreadPool::GetChannelIndustrytype(int iChannelId)
{
    channelMap_t::iterator iter = m_ChannelMap.find(iChannelId);
    if(iter == m_ChannelMap.end())
    {
        channelMap_t::iterator iter1 = m_ChannelSgipMap.find(iChannelId);
        if (iter1 != m_ChannelSgipMap.end() && iter1->second.httpmode == 5)
        {
            return iter1->second.industrytype;
        }
        else
        {
            LogWarn("Channel[%d] not found in m_ChannelMap and m_ChannelSgipMap", iChannelId);
            return 99;
        }
    }

    return iter->second.industrytype;
}

/*会话请求超时*/
void CChannelThreadPool::HandleTimeOut( TMsg* pMsg )
{
    /* 总会话超时*/
    if( pMsg->m_iSeq  == CC_TX_CHANNEL_SESSION_TIMER_ID )
    {
        channeSessionMap_t::iterator itr =  m_mapSessions.find( pMsg->m_strSessionID );
        if( itr != m_mapSessions.end())
        {
            vector < string > contents;
            channelTxSessions *Session = itr->second;
            smsp::SmsContext *pSms = Session->m_context;
            UInt32 cnt = 0;
            if( ! pSms->m_strContent.empty())
            {
                cnt = smsTxParseMsgContent( pSms->m_strContent, contents, pSms->m_strSign, pSms->m_ulongsms, 70 );
                int msgTotal = contents.size();

                /*  获取计费条数*/
                if( msgTotal > 1 )
                {
                    pSms->m_iSmscnt = 1;
                }
                else
                {
                    pSms->m_iSmscnt = cnt;
                }

                /*设置是否为长短信*/
                if ( cnt > 1 )
                {
                    pSms->m_ulongsms = 1;
                }
                else
                {
                    pSms->m_ulongsms = 0;
                }
                pSms->m_uDivideNum  = msgTotal;
                pSms->m_uChannelCnt = cnt;
            }

            /* 处理超时*/
            for( UInt32 i =0; i< cnt; i++ )
            {
                CSMSTxStatusReport reportStatus;
                reportStatus.m_strSmsUuid = pSms->m_strSmsUuid.substr(0, 10) + getUUID();
                reportStatus.m_status = SMS_STATUS_SUBMIT_SUCCESS;
                reportStatus.m_strSubmit = TIME_OUT;
                reportStatus.m_lSubretDate = time(NULL);
                reportStatus.m_strContent  = contents.at(i);
                smsTxSendFailReport( Session,  &reportStatus );
                LogWarn("Session Timeout [%s:%s], Contenx:%s",
                        pMsg->m_strSessionID.c_str(),
                        pSms->m_strPhone.c_str(),
                        reportStatus.m_strContent.c_str());
            }

            SESSION_FREE(Session);
            m_mapSessions.erase(itr);

        }
        else
        {
            LogWarn( "Can't Find Session %s", pMsg->m_strSessionID.c_str());
        }
    }
    /*超时检查滑窗*/
    else if ( pMsg->m_iSeq  == CC_TX_CHANNEL_FLOW_RESET_TIMER_ID )
    {
        SAFE_DELETE( m_pFlowWheelTime );
        smsTxFlowCtrCheckAndReset();
        m_pFlowWheelTime = SetTimer( CC_TX_CHANNEL_FLOW_RESET_TIMER_ID, "", CC_TX_CHANNEL_WINDOWSIZE_CHECK_INTERVAL );
    }
    else if(pMsg->m_iSeq  == CC_TX_CHANNEL_SYSN_SLIDEWND_TIMER_ID)
    {
        SysnSlideWndFromChannel();
        SAFE_DELETE(m_timerSynSlideWnd);
        m_timerSynSlideWnd = SetTimer(CC_TX_CHANNEL_SYSN_SLIDEWND_TIMER_ID, "", CC_TX_CHANNEL_SYSN_SLIDEWND_TIME);
    }
}

//直连协议，Pool本地滑窗为0超过5分钟，如果检测到底层滑窗不为0，将本地滑窗与底层滑窗同步, 定时器30s一次
void CChannelThreadPool::SysnSlideWndFromChannel()
{
    channelMap_t::iterator iterMap;
    time_t now = time(NULL);
    int SumWnd = 0;
    int tempWnd = 0;

    for(channelTxFlowCtr_t::iterator iterFlow = m_mapChannelFlowCtr.begin(); iterFlow != m_mapChannelFlowCtr.end(); iterFlow++)
    {
        iterMap = m_ChannelMap.find(iterFlow->first);
        if(iterMap != m_ChannelMap.end())
        {
            if(iterMap->second.httpmode > HTTP_POST)
            {
                if(    iterFlow->second->uLastWndZeroTime != 0
                    && now - iterFlow->second->uLastWndZeroTime > CC_TX_CHANNEL_SLIDEWND_0_TIME_GAP
                    && iterFlow->second->iWindowSize <= 0)
                //if(1)//for debug
                {
                    channeThreadMap_t::iterator iter = m_mapChanneThread.find(iterFlow->first);
                    if(iter != m_mapChanneThread.end())
                    {
                        SumWnd = 0;
                        for(channeThreads_t::iterator it = iter->second.m_vecThreads.begin(); it != iter->second.m_vecThreads.end(); it++)
                        {
                            tempWnd = 0;
                            if(CHANNEL_FLOW_STATE_NORMAL == static_cast<CDirectSendThread*>(*it)->GetFlowState(iterFlow->first, tempWnd))
                            {
                                SumWnd += tempWnd;
                            }
                        }

                        if(SumWnd > 0)
                        {
                            LogWarn("Channel[%d] local Wnd Lasts %d for more than 1 min, but sum of IChannel Wnd is %d, Now Reset it!",
                                iterFlow->first, iterFlow->second->iWindowSize, SumWnd);

                            iterFlow->second->iWindowSize = SumWnd;
                            CChanelTxFlowCtr::channelFlowCtrSetSize(iterFlow->first, SumWnd);
                        }
                    }
                }
            }
        }
    }
}

/* 解析短信内容*/
UInt32 CChannelThreadPool::smsTxParseMsgContent(string& content, vector<string>& contents,
                          string& sign, UInt32 uLongSms, UInt32 uSmslength )
{
    utils::UTFString utfHelper;

    UInt32 msgLength = utfHelper.getUtf8Length(content);
    UInt32 signLen = utfHelper.getUtf8Length(sign);

    if ((uLongSms == 0) && (msgLength > (70 - signLen)))    //not support long msg   //70       ||160
    {
        int lastIndex = 0;
        while (msgLength > 0)
        {
            int count = std::min((70 - signLen), msgLength);
            std::string substr;
            utfHelper.subUtfString(content, substr, lastIndex, lastIndex + count);
            lastIndex += count;
            msgLength -= count;
            contents.push_back(substr);
        }
        return contents.size();
    }
    else        //support long msg
    {
        contents.push_back( content );

        if(msgLength + signLen <= 70)   //70    ||160
        {
            return 1;
        }

        UInt32 cnt = (msgLength+signLen)/67;    //67
        if((msgLength+signLen)*1.0/67 == cnt)   //67
        {
            return cnt;
        }
        else
        {
            return cnt + 1;
        }
    }
}

bool CChannelThreadPool::smsTxGetSignExtGw( smsDirectInfo& smsParam )
{
    string strKey = "";
    strKey.append(Comm::int2str(smsParam.m_iChannelId));
    strKey.append("&");
    strKey.append(smsParam.m_strUcpaasPort);


    SignExtnoGwMap::iterator iter = m_mapSignExtGw.find(strKey);
    if(iter != m_mapSignExtGw.end())
    {
        smsParam.m_strFee_Terminal_Id.append(iter->second[0].m_strFee_Terminal_Id);
        LogNotice("[ :%s] key[%s] is find in t_signextno_gw,Fee_Terminal_Id[%s]",
                  smsParam.m_strPhone.c_str(),strKey.c_str(),
                  smsParam.m_strFee_Terminal_Id.c_str());
    }
    else
    {
        LogWarn("[ :%s] key[%s] is not find t_signextno_gw",
                 smsParam.m_strPhone.c_str(),strKey.c_str());
        smsParam.m_strFee_Terminal_Id.append("");
        return false;
    }

    return true;
}


/*保存请求消息*/
channelTxSessions* CChannelThreadPool::smsTxSaveReqMsg( TMsg* pMsg )
{
    channelTxSessions* pSession = new channelTxSessions();
    smsp::SmsContext*  pSms   = pSession->m_context;

    TDispatchToDirectSendReqMsg* pDMsg = ( TDispatchToDirectSendReqMsg*)pMsg;

    pSession->m_reqMsg = new smsDirectInfo( pDMsg->m_smsParam);
    pSession->m_bDirectProt = true;
    pSession->m_uPhoneCnt = 1;

    channelMap_t::iterator itrChann = m_ChannelMap.find( pDMsg->m_smsParam.m_iChannelId );
    if( itrChann == m_ChannelMap.end() ){
        LogError("Channel[%d] Not Exsit", pDMsg->m_smsParam.m_iChannelId );
        return NULL;
    }

    /*重新赋值归属销售*/
    std::map<std::string,SmsAccount>::iterator iterAccount = m_accountMap.find( pDMsg->m_smsParam.m_strClientId);
    if( iterAccount != m_accountMap.end()) {
        pDMsg->m_smsParam.m_uBelong_sale = iterAccount->second.m_uBelong_sale;
    }

    string strShowPhone = itrChann->second._accessID + pDMsg->m_smsParam.m_strUcpaasPort + pDMsg->m_smsParam.m_strSignPort + pDMsg->m_smsParam.m_strDisplayNum;

    UInt32 uPortLength = itrChann->second._accessID.size() + itrChann->second.m_uExtendSize;

    if (  uPortLength > 21 )
    {
         uPortLength = 21;
    }

    if ( strShowPhone.length() >  uPortLength )
    {
        strShowPhone = strShowPhone.substr( 0, uPortLength );
    }

    if( CMPP_FUJIAN == itrChann->second.m_uProtocolType )
    {
        smsTxGetSignExtGw( pDMsg->m_smsParam);
    }

    LogDebug("ClientId:%s, Context:%s, Phone:%s, strShowPhone:%s",
             pDMsg->m_smsParam.m_strClientId.c_str(),
             pDMsg->m_smsParam.m_strContent.c_str(),
             pDMsg->m_smsParam.m_strPhone.c_str(),
             strShowPhone.data());

    pSms->m_strChannelname        = itrChann->second.channelname;
    pSms->m_uChannelOperatorsType = itrChann->second.operatorstyle;
    pSms->m_strChannelRemark      = itrChann->second.m_strRemark;
    pSms->m_uChannelIdentify      = itrChann->second.m_uChannelIdentify;


    pSms->m_iChannelId     = pDMsg->m_smsParam.m_iChannelId;
    pSms->m_iOperatorstype = pDMsg->m_smsParam.m_uPhoneType;
    pSms->m_strSmsId       = pDMsg->m_smsParam.m_strSmsId;
    pSms->m_strPhone       = pDMsg->m_smsParam.m_strPhone;
    pSms->m_strContent     = pDMsg->m_smsParam.m_strContent;
    pSms->m_iSmsfrom       = pDMsg->m_smsParam.m_uSmsFrom;
    pSms->m_fCostFee       = pDMsg->m_smsParam.m_fCostFee;
    pSms->m_strClientId    = pDMsg->m_smsParam.m_strClientId;

    pSms->m_uPayType       = pDMsg->m_smsParam.m_uPayType;
    pSms->m_lSendDate      = pDMsg->m_smsParam.m_uSendDate;
    pSms->m_strUserName    = pDMsg->m_smsParam.m_strUserName;
    pSms->m_strShowPhone   = strShowPhone;
    pSms->m_strC2sTime     = pDMsg->m_smsParam.m_strC2sTime;
    pSms->m_lSubmitDate    = time(NULL);

    pSms->m_uClientCnt     = pDMsg->m_smsParam.m_uClientCnt; /*客户端条数*/
    pSms->m_iArea          = pDMsg->m_smsParam.m_uArea;
    pSms->m_uC2sId         = pDMsg->m_smsParam.m_uC2sId;
    pSms->m_strTemplateId  = pDMsg->m_smsParam.m_strTemplateId;
    pSms->m_strTemplateParam      = pDMsg->m_smsParam.m_strTemplateParam;
    pSms->m_strChannelTemplateId  = pDMsg->m_smsParam.m_strChannelTemplateId;
    pSms->m_strDisplaynum         = pDMsg->m_smsParam.m_strDisplayNum;
    pSms->m_uBelong_business      = pDMsg->m_smsParam.m_uBelong_business;
    pSms->m_uBelong_sale          = pDMsg->m_smsParam.m_uBelong_sale;

    pSms->m_strSign               = pDMsg->m_smsParam.m_strSign;
    pSms->m_uSmsType              = pDMsg->m_smsParam.m_uSmsType;
    pSms->m_ulongsms              = pDMsg->m_smsParam.m_uLongSms; /*通道是否支持长短信表示*/
    pSms->m_uShowSignType         = pDMsg->m_smsParam.m_uShowSignType;
    pSms->m_strAccessids          = pDMsg->m_smsParam.m_strAccessids;
    pSms->m_strSignPort           = pDMsg->m_smsParam.m_strSignPort;
    pSms->m_uHttpmode             = pDMsg->m_smsParam.m_uHttpmode;
    pSms->m_uResendTimes          = pDMsg->m_smsParam.m_uResendTimes;
    pSms->m_strFailedChannel      = pDMsg->m_smsParam.m_strFailedChannel;
    pSms->m_strShowSignType_c2s   = pDMsg->m_smsParam.m_strShowSignType_c2s;
    pSms->m_strSignExtendPort_c2s = pDMsg->m_smsParam.m_strSignExtendPort_c2s;
    pSms->m_strUserExtendPort_c2s = pDMsg->m_smsParam.m_strUserExtendPort_c2s;
    pSms->m_bResendCfgFlag = smsTxCheckFailedResendCfg(pDMsg->m_smsParam);
    pSms->m_strChannelOverrate_access = pDMsg->m_smsParam.m_strChannelOverrate_access;
    pSms->m_oriChannelId              = Comm::strToUint32(pDMsg->m_smsParam.m_strOriChannelId);
    m_mapSessions[ pSms->m_strSmsUuid ] = pSession ;

    return pSession;

}

void CChannelThreadPool::HandleTimeOutResetSlideWindowMsg(TMsg * pMsg)
{
    TResetChannelSlideWindowMsg * p = (TResetChannelSlideWindowMsg*)pMsg;

    //处理底层有节点重置滑窗
    PoolChannelFlowCtrModify(p->m_iChannelId, p->m_iChannelWnd, GetWorkChannelNodeNum(p->m_iChannelId) * p->m_iChannelWnd);
}

void CChannelThreadPool::UpdateChannelReq(int ChannelId)
{
    channeThreadMap_t::iterator iter = m_mapChanneThread.find(ChannelId);
    if(iter != m_mapChanneThread.end())
    {
        channelMap_t::iterator iterChan = m_ChannelMap.find(ChannelId);
        if(iterChan == m_ChannelMap.end())
        {
            LogWarn("Channel[%d] not find in m_ChannelMap");
            return;
        }

        if(iterChan->second.httpmode <= HTTP_POST)
        {
            LogWarn("Channel[%d] is http", ChannelId);
            return;
        }

        for(channeThreads_t::iterator it = iter->second.m_vecThreads.begin(); it != iter->second.m_vecThreads.end(); it++)
        {
            TDirectChannelUpdateMsg* pMsg = new TDirectChannelUpdateMsg();
            pMsg->m_iChannelId = ChannelId;
            pMsg->m_Channel = iterChan->second;
            pMsg->m_iMsgType = MSGTYPE_CHANNEL_SLIDE_WINDOW_UPDATE;
            static_cast<CDirectSendThread*>(*it)->PostMsg(pMsg);
        }
    }
    else
    {
        LogWarn("Channel[%d] update SlideWindow fail!!!", ChannelId);
    }
}

/*直连通道处理*/
void CChannelThreadPool::HandleDirectSendMsg( TMsg* pMsg )
{
    channelTxSessions *pSession = NULL;
    TDispatchToDirectSendReqMsg* pDMsg = ( TDispatchToDirectSendReqMsg*)pMsg;

    smsDirectInfo *directInfo = &pDMsg->m_smsParam;
    directInfo->m_uSignMoPortExpire = m_uSignMoPortExpire;

    channeThreadMap_t::iterator itr = m_mapChanneThread.find( directInfo->m_iChannelId );
    if (itr == m_mapChanneThread.end())
    {
        LogError("m_mapChanneThread Can not find Channel[%d]!", directInfo->m_iChannelId);
        return;
    }

    CThread *pThread  = NULL;
    channeThreads_t threads = itr->second.m_vecThreads;
    UInt32 idx = itr->second.m_uPoolIdx;
    int flowCnt = 0;

    /* 记忆轮转机制*/
    for( UInt32 i = idx; i< threads.size(); i++ )
    {
        CDirectSendThread* pDirectSendThread = static_cast<CDirectSendThread*>(threads[i]);

        if (CHANNEL_FLOW_STATE_NORMAL == pDirectSendThread->GetFlowState(directInfo->m_iChannelId, flowCnt))
        {
            pThread = threads[i];
            idx = i;
            break;
        }
    }

    if( pThread == NULL )
    {
        for( UInt32 i = 0; i < idx; i++ )
        {
            CDirectSendThread* pDirectSendThread = static_cast<CDirectSendThread*>(threads[i]);

            if (CHANNEL_FLOW_STATE_NORMAL == pDirectSendThread->GetFlowState(directInfo->m_iChannelId, flowCnt))
            {
                pThread = threads[i];
                idx = i;
                break;
            }
        }
    }

    /*发送到处理线程*/
    if( pThread != NULL)
    {
        pSession = smsTxSaveReqMsg( pMsg );

        if( NULL == pSession )
        {
            LogError("[%s:%s:%s] directSend Create Session Error.",
                directInfo->m_strClientId.c_str(),
                directInfo->m_strSmsId.c_str(),
                directInfo->m_strPhone.c_str());
            return;
        }

        LogNotice("[%s:%s:%s] ChannelId[%d] Choose SendThread[%d:%u][%s], Session[%s], SlideWindow[%d]",
            pSession->m_context->m_strClientId.c_str(),
            pSession->m_context->m_strSmsId.c_str(),
            pSession->m_context->m_strPhone.c_str(),
            directInfo->m_iChannelId,
            idx,
            threads.size(),
            pThread->GetThreadName().data(),
            pSession->m_context->m_strSmsUuid.c_str(),
            flowCnt);

        TDispatchToDirectSendReqMsg *copyMsg = new TDispatchToDirectSendReqMsg();
        copyMsg->m_iMsgType = pMsg->m_iMsgType;
        copyMsg->m_smsParam = pDMsg->m_smsParam;
        copyMsg->m_strSessionID = pSession->m_context->m_strSmsUuid;
        copyMsg->m_pSender  = this;
        (static_cast<CDirectSendThread*>(pThread))->PostChannelMsg( copyMsg, directInfo->m_iChannelId);

        itr->second.m_uPoolIdx = ++idx % threads.size();

        /* 发送侧只做流水计数，不做流速判断*/
        smsTxFlowCtrSendReq( directInfo->m_iChannelId, threads.size());

    }
    else
    {
        pDMsg->m_smsParam.m_uSendTimes++;

        LogWarn("[%s:%s:%d]FlowCtr Flow Limit,SendTimes[%u]",
                directInfo->m_strSmsId.c_str(),
                directInfo->m_strPhone.c_str(),
                directInfo->m_iChannelId,
                pDMsg->m_smsParam.m_uSendTimes);

        /* 没有可用线程的
           时候设置滑窗为0 */
        //CChanelTxFlowCtr::channelFlowCtrSetSize( directInfo->m_iChannelId, 0);
        CChanelTxFlowCtr::channelFlowCtrSynchronous( directInfo->m_iChannelId, 0 );

        /* 设置时间，超时后需要重置*/
        channelTxFlowCtr_t::iterator flowItr = m_mapChannelFlowCtr.find( directInfo->m_iChannelId );
        if( flowItr != m_mapChannelFlowCtr.end())
        {
            channelTxFlowCtr *txFlowCtr = flowItr->second;
            txFlowCtr->iWindowSize = 0;
            txFlowCtr->lLastRspTime = time(NULL);
        }

        if(pDMsg->m_smsParam.m_uSendTimes >= m_uMaxSendTimes)
        {
            pSession = smsTxSaveReqMsg( pMsg );

            LogWarn("[%s:%s:%d] try %u times, now send to reback",
                directInfo->m_strSmsId.c_str(), directInfo->m_strPhone.c_str(), directInfo->m_iChannelId, m_uMaxSendTimes);


            if( NULL == pSession )
            {
                LogError("[%s:%s] directSend Create Session Error",
                         directInfo->m_strSmsId.c_str(),
                         directInfo->m_strPhone.c_str());
                return;
            }

            //丢到reback
            CSMSTxStatusReport reportStatus;
            smsTxSendToReBack(pSession, &reportStatus);
        }
        else
        {
            TDispatchToDirectSendReqMsg* msg = new TDispatchToDirectSendReqMsg();
            msg->m_smsParam = pDMsg->m_smsParam;
            msg->m_iMsgType = MSGTYPE_DISPATCH_TO_DIRECTSEND_REQ;
            this->PostMsg(msg);
        }
    }
}

void CChannelThreadPool::processChannelReq( TMsg* pMsg )
{
    web::HttpParams param;
    param.Parse(pMsg->m_strSessionID);
    LogDebug("==send== get one channel mq message data:%s.",pMsg->m_strSessionID.data());

    string strClientId = param._map["clientId"];
    string strUserName = Base64::Decode(param._map["userName"]);
    string strSmsId = param._map["smsId"];
    string strPhone = param._map["phone"];
    string strSign = Base64::Decode(param._map["sign"]);
    string strContent = Base64::Decode(param._map["content"]);
    UInt32 uSmsType = atoi(param._map["smsType"].data());
    UInt32 uSmsFrom = atoi(param._map["smsfrom"].data());
    UInt32 uPayType = atoi(param._map["paytype"].data());
    /* from c2s for reback mq add by ljl 2017-01-25*/
    string strShowSignType_c2s = param._map["showsigntype"];
    string strUserExtendPort_c2s = param._map["userextpendport"];
    string strSignExtendPort_c2s = param._map["signextendport"];

    UInt32 uShowSignType = atoi(param._map["showsigntype_ex"].data());
    string strUcpaasPort = param._map["ucpaasport"];
    /////te su chuli signport and userextendport
    string strSignExtendPort = param._map["signport_ex"];
    string strUserExtendPort = param._map["userextno_ex"];
    UInt64 uAccessId = atol(param._map["accessid"].data());
    UInt32 uArea = atoi(param._map["area"].data());
    UInt32 uChannelId = atoi(param._map["channelid"].data());
    double uSaleFee = atof(param._map["salefee"].data());
    float  fCostFee = atof(param._map["costfee"].data());
    UInt32 uOperator = atoi(param._map["operater"].data());
    string strIds = param._map["ids"];
    UInt32 uClientCnt = atoi(param._map["clientcnt"].data());
    UInt32 uProcess_times = atoi(param._map["process_times"].data());
    UInt64 uC2sId = atol(param._map["csid"].data());
    string strCsTime = param._map["csdate"];
    string strErrorDate = param._map["errordate"];

    string strHttpOauthUrl = param._map["oauth_url"];
    strHttpOauthUrl = smsp::Channellib::decodeBase64(strHttpOauthUrl.data(), strHttpOauthUrl.length());
    string strHttpOauthData = param._map["oauth_post_data"];
    strHttpOauthData = smsp::Channellib::decodeBase64(strHttpOauthData.data(), strHttpOauthData.length());

    string strTemplateId = param._map["templateid"];
    string strTemplateParam = param._map["temp_params"];
    string strChannelTemplateId = param._map["channel_tempid"];

    UInt32 uResendTimes = atoi(param._map["failed_resend_times"].data());
    string strFailedChannel = param._map["failed_resend_channel"];

    string strChannelOverDate = param._map["channelOverrateDate"];

    string strOriChannelId = param._map["oriChannelid"];


    map<int, models::Channel>::iterator itrChann = m_ChannelMap.find(uChannelId);
    if(itrChann == m_ChannelMap.end())
    {
        LogFatal("Channel[%d] not found in m_mapChannelInfo", uChannelId);
        return;
    }

    if((itrChann->second.httpmode == YD_CMPP) || (itrChann->second.httpmode == DX_SMGP) || (itrChann->second.httpmode == LT_SGIP)
        || (itrChann->second.httpmode == GJ_SMPP) || (itrChann->second.httpmode == YD_CMPP3))
    {
        ////direct channel
        TDispatchToDirectSendReqMsg* msg = new TDispatchToDirectSendReqMsg();
        msg->m_smsParam.m_strClientId.assign(strClientId);
        msg->m_smsParam.m_strUserName.assign(strUserName);
        msg->m_smsParam.m_strSmsId.assign(strSmsId);
        msg->m_smsParam.m_strPhone.assign(strPhone);
        msg->m_smsParam.m_strSign.assign(strSign);
        msg->m_smsParam.m_strContent.assign(strContent);
        msg->m_smsParam.m_uSmsType = uSmsType;
        msg->m_smsParam.m_uSmsFrom = uSmsFrom;
        msg->m_smsParam.m_uPayType = uPayType;
        msg->m_smsParam.m_uShowSignType = uShowSignType;
        msg->m_smsParam.m_strUcpaasPort.assign(strUcpaasPort);
        msg->m_smsParam.m_strSignPort.assign(strSignExtendPort);
        msg->m_smsParam.m_strDisplayNum.assign(strUserExtendPort);
        msg->m_smsParam.m_uAccessId = uAccessId;
        msg->m_smsParam.m_uArea = uArea;
        msg->m_smsParam.m_iChannelId = uChannelId;
        msg->m_smsParam.m_lSaleFee = uSaleFee;
        msg->m_smsParam.m_fCostFee = fCostFee;
        msg->m_smsParam.m_uPhoneType = uOperator;
        msg->m_smsParam.m_strAccessids.assign(strIds);
        msg->m_smsParam.m_uClientCnt = uClientCnt;
        msg->m_smsParam.m_uProcessTimes = uProcess_times;
        msg->m_smsParam.m_uC2sId = uC2sId;
        msg->m_smsParam.m_strC2sTime = strCsTime;
        msg->m_smsParam.m_uSendDate = time(NULL);
        msg->m_smsParam.m_uChannelIdentify = itrChann->second.m_uChannelIdentify;
        msg->m_smsParam.m_strErrorDate.assign(strErrorDate);

        msg->m_smsParam.m_strTemplateId.assign(strTemplateId);
        msg->m_smsParam.m_strTemplateParam.assign(strTemplateParam);
        msg->m_smsParam.m_strChannelTemplateId.assign(strChannelTemplateId);

        msg->m_smsParam.m_uBelong_business = itrChann->second.m_uBelong_business;
        msg->m_smsParam.m_uHttpmode = itrChann->second.httpmode;

        msg->m_smsParam.m_uResendTimes = uResendTimes;
        msg->m_smsParam.m_strFailedChannel = strFailedChannel;
        msg->m_smsParam.m_strShowSignType_c2s = strShowSignType_c2s;
        msg->m_smsParam.m_strUserExtendPort_c2s = strUserExtendPort_c2s;
        msg->m_smsParam.m_strSignExtendPort_c2s = strSignExtendPort_c2s;
        msg->m_smsParam.m_strChannelOverrate_access = strChannelOverDate;
        msg->m_smsParam.m_strOriChannelId = strOriChannelId;
        msg->m_iMsgType = MSGTYPE_DISPATCH_TO_DIRECTSEND_REQ;
        smsTxDirectChannelReq( msg );
        SAFE_DELETE(msg);

    }
    else
    {
        ////http channel
        TDispatchToHttpSendReqMsg* msg = new TDispatchToHttpSendReqMsg();
        msg->m_smsParam.m_strClientId.assign(strClientId);
        msg->m_smsParam.m_strUserName.assign(strUserName);
        msg->m_smsParam.m_strSmsId.assign(strSmsId);
        msg->m_smsParam.m_strPhone.assign(strPhone);
        msg->m_smsParam.m_strSign.assign(strSign);
        msg->m_smsParam.m_strContent.assign(strContent);
        msg->m_smsParam.m_uSmsType = uSmsType;
        msg->m_smsParam.m_uSmsFrom = uSmsFrom;
        msg->m_smsParam.m_uPayType = uPayType;
        msg->m_smsParam.m_uShowSignType = uShowSignType;
        msg->m_smsParam.m_strUcpaasPort.assign(strUcpaasPort);
        msg->m_smsParam.m_strSignPort.assign(strSignExtendPort);
        msg->m_smsParam.m_strDisplayNum.assign(strUserExtendPort);
        msg->m_smsParam.m_uAccessId = uAccessId;
        msg->m_smsParam.m_uArea = uArea;
        msg->m_smsParam.m_iChannelId = uChannelId;
        msg->m_smsParam.m_lSaleFee = uSaleFee;
        msg->m_smsParam.m_fCostFee = fCostFee;
        msg->m_smsParam.m_uPhoneType = uOperator;
        msg->m_smsParam.m_strAccessids.assign(strIds);
        msg->m_smsParam.m_uClientCnt = uClientCnt;
        msg->m_smsParam.m_uProcessTimes = uProcess_times;
        msg->m_smsParam.m_uC2sId = uC2sId;
        msg->m_smsParam.m_strC2sTime = strCsTime;
        msg->m_smsParam.m_uSendDate = time(NULL);
        msg->m_smsParam.m_strErrorDate.assign(strErrorDate);

        ////http protocal must
        msg->m_smsParam.m_strCodeType.assign(itrChann->second.coding);
        msg->m_smsParam.m_strChannelName.assign(itrChann->second.channelname);
        msg->m_smsParam.m_strSendUrl.assign(itrChann->second.url);
        msg->m_smsParam.m_strSendData.assign(itrChann->second.postdata);
        msg->m_smsParam.m_uSendType = itrChann->second.httpmode;
        msg->m_smsParam.m_uExtendSize = itrChann->second.m_uExtendSize;
        msg->m_smsParam.m_uChannelType = itrChann->second.m_uChannelType;
        msg->m_smsParam.m_uChannelOperatorsType = itrChann->second.operatorstyle;
        msg->m_smsParam.m_strChannelRemark = itrChann->second.m_strRemark;
        msg->m_smsParam.m_uChannelIdentify = itrChann->second.m_uChannelIdentify;
        msg->m_smsParam.m_uLongSms = itrChann->second.longsms;
        msg->m_smsParam.m_strChannelLibName = itrChann->second.m_strChannelLibName;

        msg->m_smsParam.m_strOauthUrl.assign(strHttpOauthUrl);
        msg->m_smsParam.m_strOauthData.assign(strHttpOauthData);
        msg->m_smsParam.m_strTemplateId.assign(strTemplateId);
        msg->m_smsParam.m_strTemplateParam.assign(strTemplateParam);
        msg->m_smsParam.m_strChannelTemplateId.assign(strChannelTemplateId);

        msg->m_smsParam.m_uBelong_business = itrChann->second.m_uBelong_business;
        msg->m_smsParam.m_uHttpmode = itrChann->second.httpmode;

        msg->m_smsParam.m_uResendTimes = uResendTimes;
        msg->m_smsParam.m_strFailedChannel = strFailedChannel;
        msg->m_smsParam.m_strShowSignType_c2s = strShowSignType_c2s;
        msg->m_smsParam.m_strUserExtendPort_c2s = strUserExtendPort_c2s;
        msg->m_smsParam.m_strSignExtendPort_c2s = strSignExtendPort_c2s;
        msg->m_smsParam.m_strChannelOverrate_access = strChannelOverDate;
        msg->m_smsParam.m_strOriChannelId = strOriChannelId;
        msg->m_iMsgType = MSGTYPE_DISPATCH_TO_HTTPSEND_REQ;
        smsTxHttpChannelReq( msg );
        SAFE_DELETE(msg);

    }

}

void CChannelThreadPool::smsTxDirectChannelReq( TMsg* pMsg )
{
    if (m_uSendAuditInterceptSwitch)
    {
        GetSendAuditInterceptRedis(pMsg, DIRECT);
    }
    else
    {
        HandleDirectSendMsg(pMsg);
    }
}

/*将消息发送到对应的处理对象*/
UInt32 CChannelThreadPool::HandleHttpSendMsg( TMsg* pMsg )
{
    UInt32 ret = false;
    CThread* pThread = NULL;
    int idx  = 0;

    TDispatchToHttpSendReqMsg *httpReq = ( TDispatchToHttpSendReqMsg * )pMsg;
    if( NULL == httpReq ){
        LogError("Get Msg Error NULL");
        return ret;
    }

    /*获取通道属性*/
    map<int, models::Channel>::iterator cItr = m_ChannelMap.find( httpReq->m_smsParam.m_iChannelId );
    if( cItr == m_ChannelMap.end()){
        LogWarn("[%s:%s:%d] Request Error: Channel May Closed ",
                  httpReq->m_smsParam.m_strSmsId.c_str(),
                  httpReq->m_smsParam.m_strPhone.c_str(),
                  httpReq->m_smsParam.m_iChannelId );
        return CC_TX_RET_CHANNEL_CLOSED;
    }

    std::map<std::string,SmsAccount>::iterator iterAccount = m_accountMap.find( httpReq->m_smsParam.m_strClientId );
    if( iterAccount != m_accountMap.end()) {
        httpReq->m_smsParam.m_uBelong_salse= iterAccount->second.m_uBelong_sale;
    }

    httpReq->m_smsParam.m_bResendCfgFlag = smsTxCheckFailedResendCfg(httpReq->m_smsParam);
    httpReq->m_smsParam.m_uSignMoPortExpire = m_uSignMoPortExpire;
    httpReq->m_smsParam.m_Channel = cItr->second;

    /*组包发送*/
    if( cItr->second.m_uClusterType == SMS_CLUSTER_TYPE_NUM_MUTI   /*通道组包发送*/
        && (cItr->second.m_uclusterMaxNumber > 0)
        && (cItr->second.m_uClusterMaxTime > 0)
        && (httpReq->m_strSessionID != SMS_CLUSTER_TX_SEND )
        && (httpReq->m_strSessionID != SMS_CLUSTER_TX_REBACK ))
    {
        TDispatchToHttpSendReqMsg *ClusterMsg = new TDispatchToHttpSendReqMsg();
        ClusterMsg->m_iMsgType = MSGTYPE_DISPATCH_TO_HTTPSEND_REQ;
        ClusterMsg->m_smsParam = httpReq->m_smsParam;
        ClusterMsg->m_smsParam.m_uClusterType = cItr->second.m_uClusterType;
        ClusterMsg->m_smsParam.m_uclusterMaxNumber = cItr->second.m_uclusterMaxNumber;
        ClusterMsg->m_smsParam.m_uClusterMaxTime   = cItr->second.m_uClusterMaxTime;
        ClusterMsg->m_pSender   = this;
        ret = g_CClusterThread->PostMsg( ClusterMsg );
        return ret;
    }

    channelTxSessions* pSession = new channelTxSessions();
    smsp::SmsContext*  pSms   = pSession->m_context;

    pSession->m_reqMsg = new SmsHttpInfo( httpReq->m_smsParam );

    string strShowPhone = httpReq->m_smsParam.m_strUcpaasPort + httpReq->m_smsParam.m_strSignPort + httpReq->m_smsParam.m_strDisplayNum;
    if (httpReq->m_smsParam.m_uExtendSize > 21)
    {
        httpReq->m_smsParam.m_uExtendSize = 21;
    }

    if ( strShowPhone.length() > httpReq->m_smsParam.m_uExtendSize )
    {
        strShowPhone = strShowPhone.substr( 0,httpReq->m_smsParam.m_uExtendSize );
    }

    LogDebug("ClientId:%s, Context:%s, Phone:%s",
            httpReq->m_smsParam.m_strClientId.c_str(),
            httpReq->m_smsParam.m_strContent.c_str(),
            httpReq->m_smsParam.m_strPhone.c_str());

    pSms->m_iChannelId     = httpReq->m_smsParam.m_iChannelId;
    pSms->m_strChannelname = httpReq->m_smsParam.m_strChannelName;
    pSms->m_iOperatorstype = httpReq->m_smsParam.m_uPhoneType;
    pSms->m_strSmsId       = httpReq->m_smsParam.m_strSmsId;
    pSms->m_strPhone       = httpReq->m_smsParam.m_strPhone;
    pSms->m_strContent     = httpReq->m_smsParam.m_strContent;
    pSms->m_iSmsfrom       = httpReq->m_smsParam.m_uSmsFrom;
    pSms->m_fCostFee       = httpReq->m_smsParam.m_fCostFee;

    pSms->m_strClientId.assign(httpReq->m_smsParam.m_strClientId);

    pSms->m_uPayType       = httpReq->m_smsParam.m_uPayType;
    pSms->m_lSendDate      = httpReq->m_smsParam.m_uSendDate;
    pSms->m_strUserName    = httpReq->m_smsParam.m_strUserName;
    pSms->m_strShowPhone   = strShowPhone;
    pSms->m_strC2sTime     = httpReq->m_smsParam.m_strC2sTime;
    pSms->m_lSubmitDate    = time(NULL);

    pSms->m_uChannelOperatorsType = httpReq->m_smsParam.m_uChannelOperatorsType;
    pSms->m_strChannelRemark      = httpReq->m_smsParam.m_strChannelRemark;
    pSms->m_uChannelIdentify      = httpReq->m_smsParam.m_uChannelIdentify;
    pSms->m_uClientCnt            = httpReq->m_smsParam.m_uClientCnt; /*客户端条数*/
    pSms->m_iArea                 = httpReq->m_smsParam.m_uArea;
    pSms->m_uC2sId                = httpReq->m_smsParam.m_uC2sId;
    pSms->m_strTemplateId         = httpReq->m_smsParam.m_strTemplateId;
    pSms->m_strTemplateParam      = httpReq->m_smsParam.m_strTemplateParam;
    pSms->m_strChannelTemplateId  = httpReq->m_smsParam.m_strChannelTemplateId;
    pSms->m_strMasUserId          = httpReq->m_smsParam.m_strMasUserId;
    pSms->m_strMasAccessToken     = httpReq->m_smsParam.m_strMasAccessToken;
    pSms->m_strDisplaynum         = httpReq->m_smsParam.m_strDisplayNum;

    pSms->m_uBelong_business      = httpReq->m_smsParam.m_uBelong_business;
    pSms->m_uBelong_sale          = httpReq->m_smsParam.m_uBelong_salse;

    pSms->m_strSign               = httpReq->m_smsParam.m_strSign;
    pSms->m_uSmsType              = httpReq->m_smsParam.m_uSmsType;
    pSms->m_ulongsms              = httpReq->m_smsParam.m_uLongSms; /*通道是否支持长短信表示*/
    pSms->m_strAccessids          = httpReq->m_smsParam.m_strAccessids;
    pSms->m_uShowSignType         = httpReq->m_smsParam.m_uShowSignType;
    pSms->m_uHttpmode             = httpReq->m_smsParam.m_uHttpmode;
    pSms->m_uResendTimes          = httpReq->m_smsParam.m_uResendTimes;
    pSms->m_strFailedChannel      = httpReq->m_smsParam.m_strFailedChannel;
    pSms->m_bResendCfgFlag        = httpReq->m_smsParam.m_bResendCfgFlag;
    pSms->m_strShowSignType_c2s   = httpReq->m_smsParam.m_strShowSignType_c2s;
    pSms->m_strSignExtendPort_c2s = httpReq->m_smsParam.m_strSignExtendPort_c2s;
    pSms->m_strUserExtendPort_c2s = httpReq->m_smsParam.m_strUserExtendPort_c2s;
    pSms->m_strChannelOverrate_access = httpReq->m_smsParam.m_strChannelOverrate_access;
    pSms->m_oriChannelId              = Comm::strToUint32(httpReq->m_smsParam.m_strOriChannelId);
    pSession->m_uPhoneCnt         = 1;

    m_mapSessions[ pSms->m_strSmsUuid ] = pSession ;

    /*save Cluster Session Data*/
    bool bCluster = false;

    if( pMsg->m_strSessionID == SMS_CLUSTER_TX_SEND
        || pMsg->m_strSessionID == SMS_CLUSTER_TX_REBACK )
    {
        pSession->m_strClusterType = pMsg->m_strSessionID ;
        pSession->m_uClusterSession = httpReq->m_uSessionId ;
        pSession->m_iSeq     = pMsg->m_iSeq;
        pSession->m_uPhoneCnt   =httpReq->m_uPhonesCount;
        pSession->m_pSender  = pMsg->m_pSender;
        bCluster = true;
    }

    /* 设置会话超时时间*/
    pSession->m_pWheelTime = SetTimer( CC_TX_CHANNEL_SESSION_TIMER_ID,
                                     pSms->m_strSmsUuid , CC_TX_CHANNEL_SESSION_TIMEOUT_90S );
    /*选择http 处理线程*/
    if( m_poolTxThreads.size() > 0 )
    {
        if( pMsg->m_iSeq > 0 )
        {
            idx = httpReq->m_iSeq % m_poolTxThreads.size();
        }
        else
        {
            idx = m_uPollIdx++ % m_poolTxThreads.size();
        }
        pThread = m_poolTxThreads[idx];
    }

    /*短信发送*/
    if( NULL != pThread )
    {
        pSession->m_threadId = idx;
        TDispatchToHttpSendReqMsg *copyMsg = new TDispatchToHttpSendReqMsg();
        copyMsg->m_iMsgType = MSGTYPE_DISPATCH_TO_HTTPSEND_REQ;
        copyMsg->m_smsParam = httpReq->m_smsParam;
        copyMsg->m_strSessionID = pSms->m_strSmsUuid;
        copyMsg->m_pSender  = this;
        ret = pThread->PostMsg( copyMsg );
        LogDebug("Thread[cnt:%d, id:%d, size:%u ] , Session:%s, Smsid:%s, Phone:%s ",
                 m_poolTxThreads.size(),
                 idx,
                 pSession->m_uPhoneCnt,
                 pSms->m_strSmsUuid.c_str(),
                 pSms->m_strSmsId.c_str(),
                 pSms->m_strPhone.c_str());

        /* 发送侧只做流水计数，不做流速判断*/
        smsTxFlowCtrSendReq( pSms->m_iChannelId, 1,  bCluster ? 1 : pSession->m_uPhoneCnt );

        if ( bCluster ){
            CChanelTxFlowCtr::channelFlowCtrCountSub( FLOW_CTR_CALLER_POOL, pSms->m_iChannelId );
        }

    }
    else
    {
        LogWarn("Thread[cnt:%d, id:%d] [%s:%s] Can't Find Thread",
                 m_poolTxThreads.size(),
                 idx,
                 pSms->m_strSmsId.c_str(),
                 pSms->m_strPhone.c_str());
        SESSION_FREE(pSession);
    }

    return ret;
}


UInt32 CChannelThreadPool::smsTxFlowCtrAddOne( UInt32 uChannelId)
{
    channelTxFlowCtr *txFlowCtr = NULL;
    time_t now = time(NULL);

    map<int, models::Channel>::iterator itr =  m_ChannelMap.find( uChannelId );
    if( itr  == m_ChannelMap.end() )
    {
        LogWarn("TxFlowReq Channel %u Not Find", uChannelId );
        return CC_TX_RET_CHANNEL_NOT_EXSIT;
    }

    UInt32 uLinkSize = 1;
    if(itr->second.httpmode > HTTP_POST)
    {
        uLinkSize = GetWorkChannelNodeNum(uChannelId);
    }

    channelTxFlowCtr_t::iterator flowItr = m_mapChannelFlowCtr.find( uChannelId );
    if( flowItr == m_mapChannelFlowCtr.end())
    {
        LogWarn( "ChannelId :%u not Exsit", uChannelId );
        return CC_TX_RET_CHANNEL_NOT_EXSIT;
    }

    txFlowCtr = flowItr->second;
    txFlowCtr->iWindowSize = CChanelTxFlowCtr::channelFlowCtrCountAdd( uChannelId, itr->second.m_uSliderWindow * uLinkSize);
    txFlowCtr->lLastRspTime = now;

    if(txFlowCtr->iWindowSize <= 0)
    {
        txFlowCtr->uLastWndZeroTime = now;
    }

    LogDebug("FlowCtr(%u):[ uWindSize:[%d:%u], totalReqCnt:%u, totalRspCnt:%u, RetCnt:%d, ReqTimeElapse:%ld",
            uChannelId, txFlowCtr->iWindowSize, itr->second.m_uSliderWindow * uLinkSize,
            txFlowCtr->uTotalReqCnt,txFlowCtr->uTotalRspCnt,
            txFlowCtr->uResetCount,
            now - txFlowCtr->lLastReqTime);

    return CC_TX_RET_SUCCESS;

}


template<typename T>
void CChannelThreadPool::InsertDB(T& smsInfo, string strError)
{
    char sql[4096]  = {0};
    struct tm timenow = {0};
    time_t now = 0;
    if (smsInfo.m_uSendDate != 0)
    {
        now = smsInfo.m_uSendDate;
    }
    else
    {
        now = time(NULL);
    }

    char dateTime[64] = {0};
    char submitTime[64] = {0};

    smsTxFlowCtrAddOne(smsInfo.m_iChannelId);


    localtime_r(&now,&timenow);

    string strRecordDate = smsInfo.m_strC2sTime.substr(0,8);

    if (smsInfo.m_uSendDate != 0)
    {
        struct tm pTime = {0};
        localtime_r((time_t*)&smsInfo.m_uSendDate,&pTime);
        strftime(dateTime,sizeof(dateTime),"%Y%m%d%H%M%S",&pTime);
    }
    else
    {
        strftime(dateTime,sizeof(dateTime),"%Y%m%d%H%M%S",&timenow);
    }


    strftime(submitTime,sizeof(submitTime),"%Y%m%d%H%M%S",&timenow);

    //std::string msg = pSms->m_strContent;

    char contentSQL[2500] = { 0 };
    char cSign[512] = {0};
    char strTempParam[2048] = { 0 };
    UInt32 position = smsInfo.m_strSign.length();
    if(smsInfo.m_strSign.length() > 100)
    {
        position = Comm::getSubStr(smsInfo.m_strSign,100);
    }
    MYSQL* MysqlConn = g_pDisPathDBThreadPool->CDBGetConn();

    string strContent = smsInfo.m_strContent;
    if (strError == SEND_AUDIT_INTERCEPT)
    {
        bool isIncludeChinese = Comm::IncludeChinese((char*)(smsInfo.m_strSign + smsInfo.m_strContent).data());

        string strSign = "";
        string strLeft = SIGN_BRACKET_LEFT( isIncludeChinese );
        string strRight = SIGN_BRACKET_RIGHT( isIncludeChinese );

        if (!smsInfo.m_strSign.empty())
        {
            strSign = strLeft + smsInfo.m_strSign + strRight;
        }
        strContent.insert(0, strSign);
    }
    if(MysqlConn != NULL)
    {
        mysql_real_escape_string(MysqlConn, contentSQL, strContent.data(), strContent.length());
        mysql_real_escape_string(MysqlConn, cSign, smsInfo.m_strSign.data(), smsInfo.m_strSign.length());
        if (!smsInfo.m_strTemplateParam.empty())
        {
            mysql_real_escape_string(MysqlConn, strTempParam, smsInfo.m_strTemplateParam.data(),
                                 smsInfo.m_strTemplateParam.length());
        }
    }

    string strUUID = getUUID();

    string strErrorCode = "";
    string strSubmit = "";

    char strBelongBusiness[32] = {0};
    snprintf( strBelongBusiness, sizeof(strBelongBusiness)-1, "%ld", smsInfo.m_uBelong_business);

    smsTxGetSystemErrorCode(strError,strError,strErrorCode,strSubmit);
    UInt64 beLongSale;
    std::map<std::string, SmsAccount>::iterator iter = m_accountMap.find(smsInfo.m_strClientId);
    if(iter == m_accountMap.end())
    {
        beLongSale = 0;
    }
    else
    {
        beLongSale = iter->second.m_uBelong_sale;
    }

    char cBelongSale[32] = {0};
    snprintf(cBelongSale,32,"%ld",beLongSale);
    string strBelongSale = cBelongSale;

    snprintf(sql,4096,"insert into t_sms_record_%d_%s(smsuuid,channelid,channeloperatorstype,operatorstype,"
            "area,smsid,username,content,smscnt,state,phone,smsfrom,smsindex,costFee,paytype,clientid,"
            "date,submit,submitdate,send_id,longsms,clientcnt,channelcnt,senddate,template_id,channel_tempid,temp_params,belong_sale,belong_business,sign,smstype)  "
            " values('%s','%d','%d','%d','%d','%s','%s','%s','%d','%d','%s','%d','%d','%f','%d','%s',"
            "'%s','%s','%s','%lu','%d','%d','%d','%s', %s, '%s', '%s',%s, %s,'%s','%d');",
            smsInfo.m_uChannelIdentify,strRecordDate.data(),
            strUUID.data(),
            smsInfo.m_iChannelId,
            0,
            smsInfo.m_uPhoneType,
            smsInfo.m_uArea,
            smsInfo.m_strSmsId.data(),
            smsInfo.m_strUserName.data(),
            contentSQL,
            1,
            4,
            smsInfo.m_strPhone.substr(0, 20).data(),
            smsInfo.m_uSmsFrom,
            1,
            (smsInfo.m_fCostFee)*1000,
            smsInfo.m_uPayType,
            smsInfo.m_strClientId.data(),
            smsInfo.m_strC2sTime.data(),
            strSubmit.data(),
            submitTime,
            g_uComponentId,
            0,
            smsInfo.m_uClientCnt,
            1,
            dateTime,
            (true == smsInfo.m_strTemplateId.empty()) ? "NULL" : smsInfo.m_strTemplateId.data(),
            smsInfo.m_strChannelTemplateId.data(),
            strTempParam,
            (!strBelongSale.compare("0")) ? "NULL" : strBelongSale.data(),
            (0 == smsInfo.m_uBelong_business ) ? "NULL" : strBelongBusiness,
            (true == smsInfo.m_strSign.empty()) ? "NULL" : cSign,
            smsInfo.m_uSmsType);

    ////insert db
    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    pMQ->m_strExchange.assign(g_strMQDBExchange);
    pMQ->m_strRoutingKey.assign(g_strMQDBRoutingKey);
    pMQ->m_strData.assign(sql);
    pMQ->m_strData.append("RabbitMQFlag=");
    pMQ->m_strData.append(strUUID);
    g_pRecordMQProducerThread->PostMsg(pMQ);


    TSendToDeliveryReportReqMsg* pDelivery = new TSendToDeliveryReportReqMsg();
    pDelivery->m_iMsgType = MSGTYPE_SEND_TO_DELIVERYREPORT_REQ;
    pDelivery->m_strType = "1";

    pDelivery->m_strClientId.assign(smsInfo.m_strClientId);
    pDelivery->m_uChannelId = smsInfo.m_iChannelId;
    pDelivery->m_strSmsId.assign(smsInfo.m_strSmsId);
    pDelivery->m_strPhone.assign(smsInfo.m_strPhone);
    pDelivery->m_iStatus = 6;
    pDelivery->m_strDesc = strErrorCode;
    pDelivery->m_strReportDesc = strError;
    pDelivery->m_uChannelCnt = 1;
    pDelivery->m_uReportTime = time(NULL);
    pDelivery->m_uSmsFrom = smsInfo.m_uSmsFrom;
    pDelivery->m_uLongSms = 1;
    pDelivery->m_uC2sId = smsInfo.m_uC2sId;
    g_pDeliveryReportThread->PostMsg(pDelivery);

}

void CChannelThreadPool::sendAuditInterceptDirectRedisResp( TRedisResp* pRedisRes)
{
    if((NULL != pRedisRes->m_RedisReply) &&
        (pRedisRes->m_RedisReply->type == REDIS_REPLY_ARRAY && pRedisRes->m_RedisReply->elements > 0) )
    {
        InsertDB(reinterpret_cast<TDispatchToDirectSendReqMsg*>(pRedisRes->m_iSeq)->m_smsParam, SEND_AUDIT_INTERCEPT);
        string strTemp;
        paserReply("content", pRedisRes->m_RedisReply, strTemp);
        LogDebug("content[%s] is intercepted", Base64::Decode(strTemp).data());
    }
    else
    {
        if (NULL == pRedisRes->m_RedisReply)
        {
            LogError("redis return null, need to send to channel, cmd[%s]", pRedisRes->m_RedisCmd.data());
        }
        HandleDirectSendMsg(reinterpret_cast<TMsg*>(pRedisRes->m_iSeq));
    }

    delete (TMsg*)pRedisRes->m_iSeq;
    pRedisRes->m_iSeq = NULL;

}

void CChannelThreadPool::sendAuditInterceptHttpRedisResp( TRedisResp* pRedisRes)
{
    if((NULL != pRedisRes->m_RedisReply) &&
        (pRedisRes->m_RedisReply->type == REDIS_REPLY_ARRAY && pRedisRes->m_RedisReply->elements > 0) )
    {
        string strTemp;
        paserReply("content", pRedisRes->m_RedisReply, strTemp);
        LogDebug("content[%s] is intercepted", Base64::Decode(strTemp).data());
        InsertDB(reinterpret_cast<TDispatchToHttpSendReqMsg*>(pRedisRes->m_iSeq)->m_smsParam, SEND_AUDIT_INTERCEPT);
    }
    else
    {
        if (NULL == pRedisRes->m_RedisReply)
        {
            LogError("redis return null, need to send to channel, cmd[%s]", pRedisRes->m_RedisCmd.data());
        }
        HandleHttpSendMsg(reinterpret_cast<TMsg*>(pRedisRes->m_iSeq));
    }

    delete (TMsg*)pRedisRes->m_iSeq;
    pRedisRes->m_iSeq = NULL;

}

void CChannelThreadPool::HandleRedisRespMsg(TMsg* pMsg)
{
    TRedisResp* pRedisResp = (TRedisResp*)pMsg;

    if(NULL == pRedisResp)
    {
        LogError("pRedisResp is null, new memory fail in credisthread");
        return;
    }
    LogDebug("==debug== redis SessionID[%s]!",pRedisResp->m_strSessionID.data());

    if ("send_audit_intercept_http" == pRedisResp->m_strSessionID)
    {
        sendAuditInterceptHttpRedisResp(pRedisResp);
    }
    else if ("send_audit_intercept_direct" == pRedisResp->m_strSessionID)
    {
        sendAuditInterceptDirectRedisResp(pRedisResp);
    }
    else
    {
        LogError("pRedisResp m_strSessionID[%s] error", pRedisResp->m_strSessionID.c_str());
    }
    if (NULL != pRedisResp->m_RedisReply)
    {
        freeReply(pRedisResp->m_RedisReply);
    }

    return;
}

void CChannelThreadPool::GetSendAuditInterceptRedis(TMsg* pMsg, int type)
{
    TRedisReq* pRedis = new TRedisReq();
    string strRedisKey;

    if (HTTP == type)
    {
        TDispatchToHttpSendReqMsg* pDMsgHttp = (TDispatchToHttpSendReqMsg*)pMsg;

        bool isIncludeChinese = Comm::IncludeChinese((char*)(pDMsgHttp->m_smsParam.m_strSign + pDMsgHttp->m_smsParam.m_strContent).data());
        string strSign = "";
        string strLeft = SIGN_BRACKET_LEFT( isIncludeChinese );
        string strRight = SIGN_BRACKET_RIGHT( isIncludeChinese );

        if (!pDMsgHttp->m_smsParam.m_strSign.empty())
        {
            strSign = strLeft + pDMsgHttp->m_smsParam.m_strSign + strRight;
        }

        string strMd5Cal = strSign + pDMsgHttp->m_smsParam.m_strContent;
        pRedis->m_strSessionID = "send_audit_intercept_http";
        strRedisKey = "send_audit_intercept:" + Comm::GetStrMd5(strMd5Cal);
        pRedis->m_RedisCmd = "HGETALL " + strRedisKey;

        TDispatchToHttpSendReqMsg *pSendMsgHttp = new TDispatchToHttpSendReqMsg(*(TDispatchToHttpSendReqMsg*)pDMsgHttp);

        pRedis->m_iSeq = reinterpret_cast<UInt64> (pSendMsgHttp);

    }
    else if(DIRECT == type)
    {
        TDispatchToDirectSendReqMsg* pDMsgDirrect = (TDispatchToDirectSendReqMsg*)pMsg;

        bool isIncludeChinese = Comm::IncludeChinese((char*)(pDMsgDirrect->m_smsParam.m_strSign + pDMsgDirrect->m_smsParam.m_strContent).data());
        string strSign = "";
        string strLeft = SIGN_BRACKET_LEFT( isIncludeChinese );
        string strRight = SIGN_BRACKET_RIGHT( isIncludeChinese );

        if (!pDMsgDirrect->m_smsParam.m_strSign.empty())
        {
            strSign = strLeft + pDMsgDirrect->m_smsParam.m_strSign + strRight;
        }

        string strMd5Cal = strSign + pDMsgDirrect->m_smsParam.m_strContent;
        pRedis->m_strSessionID = "send_audit_intercept_direct";
        strRedisKey = "send_audit_intercept:" + Comm::GetStrMd5(strMd5Cal);
        pRedis->m_RedisCmd = "HGETALL " + strRedisKey;

        TDispatchToDirectSendReqMsg *pSendMsgDirect = new TDispatchToDirectSendReqMsg(*(TDispatchToDirectSendReqMsg*)pDMsgDirrect);

        pRedis->m_iSeq = reinterpret_cast<UInt64> (pSendMsgDirect);

    }
    else
    {
        LogError("send audit intercept type[%d] error", type);
        if (pRedis)
        {
            delete pRedis;
            pRedis = NULL;
        }
        return;
    }

    pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
    pRedis->m_pSender = this;
    pRedis->m_strKey = strRedisKey;
    SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRedis);
    LogDebug("redisCmd:%s", pRedis->m_RedisCmd.data());
    return;
}

/*将消息发送到对应的处理对象*/
UInt32 CChannelThreadPool::smsTxHttpChannelReq( TMsg* pMsg )
{
    UInt32 ret = false;

    if (m_uSendAuditInterceptSwitch)
    {
        GetSendAuditInterceptRedis(pMsg, HTTP);
    }
    else
    {
        ret = HandleHttpSendMsg(pMsg);
    }

    return ret;
}


/*
11：异常移动行业，
12：异常移动营销，
13：异常联通行业，
14：异常联通营销，
15：异常电信行业，
16：异常电信营销
map<string,ComponentRefMq> m_componentRefMqInfo;
*/
UInt32 CChannelThreadPool::smsTxSendToReBack( channelTxSessions* pSession, CSMSTxStatusReport *reportStatus )
{

    smsp::SmsContext *pSms = pSession->m_context;

    UInt32 uProcessTimes = 0;
    string strErrorDate = "";

    if(pSession->m_bDirectProt)
    {
        smsDirectInfo *pSmsDirect = (smsDirectInfo *)pSession->m_reqMsg;
        uProcessTimes = pSmsDirect->m_uProcessTimes;
        strErrorDate  = pSmsDirect->m_strErrorDate;
    }
    else
    {
        SmsHttpInfo *pSmsHttp = ( SmsHttpInfo * )pSession->m_reqMsg;
        uProcessTimes = pSmsHttp->m_uProcessTimes;
        strErrorDate  = pSmsHttp->m_strErrorDate;
    }

    string strExchange;
    string strRoutingKey;
    string strMessageType = "";
    UInt32 ret = CC_TX_RET_SUCCESS;
    string strKey = "";
    char temp[250] = {0};

    map<string,ComponentRefMq>::iterator itReq ;
    map<UInt64,MqConfig>::iterator itrMq;

    if ( YIDONG == pSms->m_iOperatorstype )
    {
        if ( SMS_TYPE_MARKETING == pSms->m_uSmsType
            || SMS_TYPE_USSD == pSms->m_uSmsType
            || SMS_TYPE_FLUSH_SMS == pSms->m_uSmsType )
        {
            strMessageType.assign("12");
        }
        else
        {
            strMessageType.assign("11");
        }
    }
    else if (DIANXIN == pSms->m_iOperatorstype )
    {
        if (SMS_TYPE_MARKETING ==  pSms->m_uSmsType
            || SMS_TYPE_USSD == pSms->m_uSmsType
            || SMS_TYPE_FLUSH_SMS == pSms->m_uSmsType )
        {
            strMessageType.assign("16");
        }
        else
        {
            strMessageType.assign("15");
        }
    }
    else if ((LIANTONG == pSms->m_iOperatorstype)
         || (FOREIGN == pSms->m_iOperatorstype))
    {
        if (SMS_TYPE_MARKETING == pSms->m_uSmsType
            || SMS_TYPE_USSD == pSms->m_uSmsType
            || SMS_TYPE_FLUSH_SMS == pSms->m_uSmsType)
        {
            strMessageType.assign("14");
        }
        else
        {
            strMessageType.assign("13");
        }
    }
    else
    {
        LogError("[%s:%s:%s] phoneType:%u is invalid.", pSms->m_strClientId.c_str(), pSms->m_strSmsId.c_str()
            , pSms->m_strPhone.c_str()
            , pSms->m_uSmsType );
        reportStatus->m_status = SMS_STATUS_SUBMIT_FAIL;
        reportStatus->m_strSubmit= PHONE_SEGMENT_ERROR;
        reportStatus->m_strYZXErrCode= PHONE_SEGMENT_ERROR;
        ret = CC_TX_RET_PARAM_ERROR;
        goto CHECK_CLUSTER;
    }

    snprintf(temp,250,"%lu_%s_0", g_uComponentId, strMessageType.data());
    strKey.assign(temp);

    itReq =  m_componentRefMqInfo.find(strKey);
    if (itReq == m_componentRefMqInfo.end())
    {
        LogError("[%s:%s:%s]strKey:%s is not find in m_componentRefMqInfo.", pSms->m_strClientId.c_str()
            , pSms->m_strSmsId.c_str(), pSms->m_strPhone.c_str() ,strKey.data());
        reportStatus->m_status = SMS_STATUS_SUBMIT_FAIL;
        reportStatus->m_strSubmit= CMPP_INTERNAL_ERROR;
        reportStatus->m_strYZXErrCode= CMPP_INTERNAL_ERROR;
        ret = CC_TX_RET_MQ_CONFIG_ERROR;
        goto CHECK_CLUSTER;
    }

    itrMq = m_mqConfigInfo.find(itReq->second.m_uMqId);
    if ( itrMq == m_mqConfigInfo.end() )
    {
        LogError("[%s:%s:%s]mqid:%lu is not find in m_mqConfigInfo.", pSms->m_strClientId.c_str()
            , pSms->m_strSmsId.c_str(), pSms->m_strPhone.c_str(), itReq->second.m_uMqId );
        reportStatus->m_status = SMS_STATUS_SUBMIT_FAIL;
        reportStatus->m_strSubmit= CMPP_INTERNAL_ERROR;
        reportStatus->m_strYZXErrCode= CMPP_INTERNAL_ERROR;
        ret = CC_TX_RET_MQ_CONFIG_ERROR;
        goto CHECK_CLUSTER;
    }

    strExchange = itrMq->second.m_strExchange;
    strRoutingKey = itrMq->second.m_strRoutingKey;

CHECK_CLUSTER:

    pSession->m_uStatus = CC_TX_STATUS_REBACK;

    if( pSession->m_strClusterType == SMS_CLUSTER_TX_SEND )
    {
        if( ret  == CC_TX_RET_SUCCESS )
        {
            smsTxStausToCluster( SMS_CLUSTER_MSG_TYPE_REBACK, pSession,
                               reportStatus, strExchange, strRoutingKey);
        }
        else
        {
            smsTxStausToCluster( SMS_CLUSTER_MSG_TYPE_FAIL, pSession,
                               reportStatus, strExchange, strRoutingKey );
        }
        return CC_TX_RET_SUCCESS;
    }
    else
    {

        if( ret !=  CC_TX_RET_SUCCESS )
        {
            smsTxSendFailReport( pSession, reportStatus );
            return CC_TX_RET_MQ_CONFIG_ERROR;
        }
    }

    ////data
    string strData = "";
    ////clientId
    strData.append("clientId=");
    strData.append(pSms->m_strClientId);
    ////userName
    strData.append("&userName=");
    strData.append(Base64::Encode(pSms->m_strUserName));
    ////smsId
    strData.append("&smsId=");
    strData.append(pSms->m_strSmsId);
    ////phone
        strData.append("&phone=");
    strData.append(pSms->m_strPhone);
    ////sign
    strData.append("&sign=");
    strData.append(Base64::Encode(pSms->m_strSign));
    ////content
    strData.append("&content=");
    strData.append(Base64::Encode(pSms->m_strContent));
    ////smsType
    strData.append("&smsType=");
    strData.append(Comm::int2str( pSms->m_uSmsType));
    ////smsfrom
    strData.append("&smsfrom=");
    strData.append(Comm::int2str(pSms->m_iSmsfrom));
    ////paytype
    strData.append("&paytype=");
    strData.append(Comm::int2str(pSms->m_uPayType));
    ////showsigntype
    strData.append("&showsigntype=");
    strData.append(pSms->m_strShowSignType_c2s);
    ////userextpendport
    strData.append("&userextpendport=");
    strData.append(pSms->m_strUserExtendPort_c2s);

    ////signport_ex
    strData.append("&signextendport=");
    strData.append(pSms->m_strSignExtendPort_c2s);

    ////operater
    strData.append("&operater=");
    strData.append(Comm::int2str(pSms->m_iOperatorstype));
    ////ids
    strData.append("&ids=");
    strData.append(pSms->m_strAccessids);
    ////clientcnt
    strData.append("&clientcnt=");
    strData.append(Comm::int2str(pSms->m_uClientCnt));
    ////process_times
    strData.append("&process_times=");
    strData.append(Comm::int2str(uProcessTimes));
    ////csid
    strData.append("&csid=");
    strData.append(Comm::int2str(pSms->m_uC2sId));
    ////csdate
    strData.append("&csdate=");
    strData.append(pSms->m_strC2sTime);
    ////errordate
    strData.append("&errordate=");
    if ( 1 == uProcessTimes )
    {
        UInt64 uNowTime = time(NULL);
        char pNow[25] = {0};
        snprintf(pNow,25,"%lu",uNowTime);
        strData.append(pNow);
    }
    else
    {
        strData.append(strErrorDate);
    }

    strData.append("&templateid=").append(pSms->m_strTemplateId);
    strData.append("&temp_params=").append(pSms->m_strTemplateParam);
    if(pSms->m_uResendTimes > 0 && !pSms->m_strFailedChannel.empty())
    {
        strData.append("&failed_resend_times=").append(Comm::int2str(pSms->m_uResendTimes));
        strData.append("&failed_resend_channel=").append(pSms->m_strFailedChannel);
    }
    if (!pSms->m_strChannelOverrate_access.empty())
    {
        strData.append("&channelOverrateDate=");
        strData.append(pSms->m_strChannelOverrate_access);
        strData.append("&oriChannelid=");
        strData.append(Comm::int2str(pSms->m_oriChannelId));
    }
    LogDebug("==http push except== exchange:%s,routingkey:%s,data:%s.",
                strExchange.data(), strRoutingKey.data(), strData.data());

    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    pMQ->m_strData = strData;
    pMQ->m_strExchange = strExchange;
    pMQ->m_strRoutingKey = strRoutingKey;
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    g_pMqAbIoProductThread->PostMsg(pMQ);

    return CC_TX_RET_SUCCESS ;
}


/*错误状态报告----> 入access 流水库*/
UInt32 CChannelThreadPool::smsTxSendFailReport( channelTxSessions *ss, CSMSTxStatusReport *reportStatus )
{
    if ( NULL == ss )
    {
        LogError("******Session NULL .******");
        return CC_TX_RET_PARAM_ERROR;
    }
    string desc;
    string reportDesc;
    smsp::SmsContext *pSmsContex = ss->m_context;

    if( !reportStatus ){
        return CC_TX_RET_PARAM_ERROR;
    }

    if( !reportStatus->m_strSubRet.empty())
    {
        string strDesc = "";
        string strReportDesc = "";
        if (pSmsContex->m_uHttpmode <= 2)//http
        {
            smsTxGetErrorCode(Comm::int2str(pSmsContex->m_iChannelId), reportStatus->m_strSubRet, strDesc, strReportDesc);
        }
        else
        {
            string directType;
            if (YD_CMPP == pSmsContex->m_uHttpmode || YD_CMPP3 == pSmsContex->m_uHttpmode)
            {
                directType.assign("cmpp");
            }
            else if (DX_SMGP == pSmsContex->m_uHttpmode)
            {
                directType.assign("smgp");
            }
            else if (LT_SGIP == pSmsContex->m_uHttpmode)
            {
                directType.assign("sgip");
            }
            else if (GJ_SMPP == pSmsContex->m_uHttpmode)
            {
                directType.assign("smpp");
            }
            else
            {
                LogError("m_uHttpmode[%u] error", pSmsContex->m_uHttpmode);
            }
            smsTxDirectGetErrorCode(directType, pSmsContex->m_iChannelId, string("1"), reportStatus->m_strSubRet, strDesc, strReportDesc);


        }

        reportStatus->m_strSubRet = strDesc ;
        desc = strDesc;
        reportDesc = strReportDesc;

    }

    if (!reportStatus->m_strSubmit.empty())
    {
        string strError = "";
        string strSubmit = "";
        smsTxGetSystemErrorCode( reportStatus->m_strYZXErrCode, reportStatus->m_strSubmit, strError, strSubmit);
        reportDesc = reportStatus->m_strSubmit;
        reportStatus->m_strSubmit     = strSubmit;

        desc = strSubmit;


    }

    ss->m_uStatus = CC_TX_STATUS_FAIL;
    ss->m_uErrCnt++;

    if( ss->m_strClusterType == SMS_CLUSTER_TX_SEND )
    {
        reportStatus->m_strYZXErrCode = reportDesc;
        smsTxStausToCluster( SMS_CLUSTER_MSG_TYPE_FAIL, ss, reportStatus, g_strMQDBExchange, g_strMQDBRoutingKey );
        return CC_TX_RET_SUCCESS;
    }

    /*失败写入record 数据库*/
    smsTxRecordInsertDb( pSmsContex, reportStatus );

    /*设置长短信标示*/
    if( ss->m_uTotalCnt > 1
        && reportStatus->m_iIndexNum == 1 )
    {
        smsTxSetLongMsgRedis( pSmsContex->m_strSmsId, pSmsContex->m_strPhone );
    }

    TSendToDeliveryReportReqMsg* pMsg = new TSendToDeliveryReportReqMsg();
    pMsg->m_iMsgType = MSGTYPE_SEND_TO_DELIVERYREPORT_REQ;
    pMsg->m_strType = "1";
    pMsg->m_strClientId.assign(pSmsContex->m_strClientId);
    pMsg->m_uChannelId = pSmsContex->m_iChannelId;
    pMsg->m_strSmsId.assign(pSmsContex->m_strSmsId);
    pMsg->m_strPhone.assign(pSmsContex->m_strPhone);

    if( reportStatus->m_status == (UInt32)SMS_STATUS_SEND_FAIL
        || reportStatus->m_status == (UInt32)SMS_STATUS_SUBMIT_FAIL)
    {
        reportStatus->m_status = SMS_STATUS_CONFIRM_FAIL;
    }

    pMsg->m_iStatus = reportStatus->m_status;
    pMsg->m_strDesc = desc;
    pMsg->m_strReportDesc = reportDesc;
    pMsg->m_uChannelCnt = pSmsContex->m_uChannelCnt;
    pMsg->m_uReportTime = time(NULL);
    pMsg->m_uSmsFrom = pSmsContex->m_iSmsfrom;
    pMsg->m_uLongSms = pSmsContex->m_uDivideNum;
    pMsg->m_uC2sId = pSmsContex->m_uC2sId;
    g_pDeliveryReportThread->PostMsg(pMsg);

    return CC_TX_RET_SUCCESS;

}


/*设置长短信标识*/
UInt32 CChannelThreadPool::smsTxSetLongMsgRedis( string strSmsId, string strPhone )
{
    TRedisReq* pDividedReq = new TRedisReq();
    string strKey = "";
    pDividedReq->m_iMsgType = MSGTYPE_REDIS_REQ;
    pDividedReq->m_strKey = "sendsms:" + strSmsId + "_" + strPhone;
    strKey = pDividedReq->m_strKey;
    pDividedReq->m_RedisCmd.assign("set ");
    pDividedReq->m_RedisCmd.append(pDividedReq->m_strKey);
    pDividedReq->m_RedisCmd.append("   1");
    SelectRedisThreadPoolIndex(g_pRedisThreadPool, pDividedReq);

    //set redis timeout
    TRedisReq* pExpireDividedReq = new TRedisReq();
    pExpireDividedReq->m_iMsgType = MSGTYPE_REDIS_REQ;
    pExpireDividedReq->m_strKey = strKey;       //share same index
    pExpireDividedReq->m_RedisCmd.assign("EXPIRE ");
    pExpireDividedReq->m_RedisCmd.append(pExpireDividedReq->m_strKey);
    pExpireDividedReq->m_RedisCmd.append(" 259200");
    SelectRedisThreadPoolIndex(g_pRedisThreadPool, pExpireDividedReq );
    LogDebug("[%s:%s] set  Long Msg Flag Param:[%s].",
             strSmsId.data(), strPhone.data(), strKey.data());

    return CC_TX_RET_SUCCESS;
}


/* 设置上行保存的redis信息*/
UInt32 CChannelThreadPool::smsTxSetMoRedisInfo( channelTxSessions *Session )
{
    smsp::SmsContext  *pSms = Session->m_context;

    SmsHttpInfo* pHttpInfo =  ( SmsHttpInfo *)Session->m_reqMsg;

    TRedisReq* req = NULL;
    const int bufsize = 2048;
    char cmd[bufsize] = {0};
    char cmdkey[1024] = {0};

    string strSign = "null";
    string strSignPort = "null";
    string strDisplayNum = "null";

    if (pHttpInfo->m_uChannelType >= 0 && pHttpInfo->m_uChannelType <= 3)
    {
        req = new TRedisReq();
        if (false == pSms->m_strSign.empty())
        {
            strSign.assign(pSms->m_strSign);
        }

        if (false == pHttpInfo->m_strSignPort.empty())
        {
            strSignPort.assign(pHttpInfo->m_strSignPort);
        }

        if (false == pHttpInfo->m_strDisplayNum.empty())
        {
            strDisplayNum.assign(pHttpInfo->m_strDisplayNum);
        }
    }

    if (0 == pHttpInfo->m_uChannelType || 3 == pHttpInfo->m_uChannelType)
    {
        /////HMSET moport:channelid_showphone client* sign* signport* userport* mourl* smsfrom*

        snprintf(cmdkey, 1024, "moport:%d_%s", pSms->m_iChannelId, pSms->m_strShowPhone.data());

        snprintf(cmd, bufsize,"HMSET %s clientid %s sign %s signport %s userport %s mourl %lu  smsfrom %d",
                cmdkey,
                pSms->m_strClientId.data(),
                strSign.data(),
                strSignPort.data(),
                strDisplayNum.data(),
                pSms->m_uC2sId,
                pSms->m_iSmsfrom);

        req->m_RedisCmd = cmd;
        req->m_iMsgType = MSGTYPE_REDIS_REQ;
        req->m_strKey = cmdkey;

        SelectRedisThreadPoolIndex(g_pRedisThreadPool, req );


        TRedisReq* pDel = new TRedisReq();
        pDel->m_RedisCmd = "EXPIRE ";
        pDel->m_RedisCmd.append(cmdkey);
        pDel->m_RedisCmd.append("  259200");
        pDel->m_iMsgType = MSGTYPE_REDIS_REQ;
        pDel->m_strKey = cmdkey;

        SelectRedisThreadPoolIndex(g_pRedisThreadPool, pDel );

        LogDebug("==http moport==smsid:%s,phone:%s,redisCmd:%s.",
                 pSms->m_strSmsId.data(), pSms->m_strPhone.data(), cmd );
    }
    else if (1 == pHttpInfo->m_uChannelType || 2 == pHttpInfo->m_uChannelType)
    {
        /////HMSET sign_mo_port:channelid_showphone:phone clientid * sign * signport * userport * mourl * smsfrom*

        snprintf(cmdkey, 1024, "sign_mo_port:%d_%s:%s", pSms->m_iChannelId, pSms->m_strShowPhone.data(),pHttpInfo->m_strPhone.data());

        snprintf(cmd, bufsize,"HMSET %s clientid %s sign %s signport %s userport %s mourl %lu  smsfrom %d",
                cmdkey,
                pSms->m_strClientId.data(),
                strSign.data(),
                strSignPort.data(),
                strDisplayNum.data(),
                pSms->m_uC2sId,
                pSms->m_iSmsfrom);

        req->m_RedisCmd = cmd;
        req->m_iMsgType = MSGTYPE_REDIS_REQ;
        req->m_strKey = cmdkey;

        SelectRedisThreadPoolIndex(g_pRedisThreadPool, req );

        LogDebug("==http sign_mo_port==smsid:%s,phone:%s,redisCmd:%s.", pSms->m_strSmsId.data(), pSms->m_strPhone.data(), cmd );

        TRedisReq* pDel = new TRedisReq();
        memset(cmd, 0x00, sizeof(cmd));
        snprintf(cmd, bufsize, "EXPIRE %s %u", cmdkey, pHttpInfo->m_uSignMoPortExpire);
        pDel->m_RedisCmd = cmd;
        pDel->m_iMsgType = MSGTYPE_REDIS_REQ;
        pDel->m_strKey = cmdkey;

        SelectRedisThreadPoolIndex(g_pRedisThreadPool, pDel );
        LogDebug("==http sign_mo_port==smsid:%s,phone:%s,redisCmd:%s.", pSms->m_strSmsId.data(), pSms->m_strPhone.data(), cmd );


    }

    return CC_TX_RET_SUCCESS;
}


/* 通道发送成功处理*/
UInt32 CChannelThreadPool::smsTxSendSuccReport( channelTxSessions *ss, CSMSTxStatusReport *reportStatus )
{
    smsp::SmsContext *pSms = ss->m_context;
    /*发送超时属于提交成功,
        但是需要增加提交失败计数*/
    bool bSubmitRespTimeout = false;
    if(TIME_OUT == reportStatus->m_strSubmit ||
        CMPP_REPLY_TIME_OUT == reportStatus->m_strSubmit ||
        SGIP_REPLY_TIME_OUT == reportStatus->m_strSubmit ||
        SMGP_REPLY_TIME_OUT == reportStatus->m_strSubmit ||
        SMPP_REPLY_TIME_OUT == reportStatus->m_strSubmit )
    {
        bSubmitRespTimeout = true;
    }

    if(bSubmitRespTimeout)
    {
        smsTxCountSubmitFailed(pSms->m_iChannelId, false );
    }else{
        /*成功后将错误计数器清空*/
        smsTxCountSubmitFailed(pSms->m_iChannelId, true );
        smsTxCountSendFailed(pSms->m_iChannelId, true );
    }

    ss->m_uStatus = CC_TX_STATUS_SUCCESS;

    if (!reportStatus->m_strSubmit.empty())
    {
        string strError = "";
        string strSubmit = "";
        smsTxGetSystemErrorCode( reportStatus->m_strYZXErrCode, reportStatus->m_strSubmit, strError, strSubmit);
        reportStatus->m_strSubmit = strSubmit;
    }

    if( ss->m_strClusterType == SMS_CLUSTER_TX_SEND )
    {
        smsTxStausToCluster( SMS_CLUSTER_MSG_TYPE_SUCCESS,  ss, reportStatus, g_strMQDBExchange, g_strMQDBRoutingKey );
        return CC_TX_RET_SUCCESS;
    }

    smsTxRecordInsertDb( pSms, reportStatus );

    if ( 0 == pSms->m_strChannelname.compare(0,9,"JXTEMPNEW"))
    {
        LogDebug("JXTEMPNEW no need to set redis");
        TSendToDeliveryReportReqMsg* pMsg = new TSendToDeliveryReportReqMsg();
        pMsg->m_iMsgType = MSGTYPE_SEND_TO_DELIVERYREPORT_REQ;
        pMsg->m_strType = "1";
        pMsg->m_strClientId.assign(pSms->m_strClientId);
        pMsg->m_uChannelId = pSms->m_iChannelId;
        pMsg->m_strSmsId.assign(pSms->m_strSmsId);
        pMsg->m_strPhone.assign(pSms->m_strPhone);
        pMsg->m_iStatus = reportStatus->m_status;
        pMsg->m_strDesc = reportStatus->m_strSubRet;
        pMsg->m_strReportDesc = reportStatus->m_strYZXErrCode;
        pMsg->m_uChannelCnt = pSms->m_uChannelCnt;
        pMsg->m_uReportTime = time(NULL);
        pMsg->m_uSmsFrom = pSms->m_iSmsfrom;
        pMsg->m_uLongSms = pSms->m_uDivideNum;
        pMsg->m_uC2sId = pSms->m_uC2sId;
        g_pDeliveryReportThread->PostMsg(pMsg);

        return CC_TX_RET_SUCCESS;
    }
    if(bSubmitRespTimeout)
    {
        LogWarn("[%s:%s] submit timeout!!",pSms->m_strSmsId.data(),pSms->m_strPhone.data());
        return CC_TX_RET_SUCCESS;
    }
    TRedisReq* pMsg = new TRedisReq();
    string strkey = "";
    if( ss->m_strClusterType == SMS_CLUSTER_TX_REBACK ){
        strkey = "cluster_sms:";
    }else{
        strkey = "sendmsgid:";
    }

    strkey.append(Comm::int2str(pSms->m_iChannelId));
    strkey.append( "_");
    strkey.append( pSms->m_strChannelSmsId ) ;

    if( ss->m_strClusterType == SMS_CLUSTER_TX_REBACK ){
        strkey.append( "_");
        strkey.append( pSms->m_strPhone );
    }

    string strCmd = "";
    strCmd.assign("HMSET  ");
    strCmd.append(strkey);
    ////sendTime
    strCmd.append("  sendTime  ");
    strCmd.append(http::UrlCode::UrlEncode(pSms->m_strC2sTime));            //http::UrlCode::UrlEncode(pSms->m_strC2sTime)
    ////smsid
    strCmd.append("  smsid ");
    strCmd.append(pSms->m_strSmsId);
    ////clientid
    strCmd.append("  clientid ");
    strCmd.append( pSms->m_strClientId );

    ////smsfrom
    strCmd.append("  smsfrom ");
    char buf3[10];
    snprintf(buf3, 10,"%d", pSms->m_iSmsfrom);
    strCmd.append(buf3);

    ////senduid
    strCmd.append("  senduid ");
    strCmd.append(reportStatus->m_strSmsUuid);
    ////longsms
    strCmd.append("  longsms ");    //拆分条数。 普通短信，拆分条数为1，长短信拆分条数为 2,3,4,5...等等。
    char buf_divide[10];
    snprintf(buf_divide, 10,"%u", pSms->m_uDivideNum);
    strCmd.append(buf_divide);
    ////phone
    strCmd.append("  phone ");
    strCmd.append(pSms->m_strPhone);
    ////c2sid
    char buf_c2sId[50] = {0};
    snprintf(buf_c2sId,50,"%lu",pSms->m_uC2sId);
    strCmd.append("  c2sid ");
    strCmd.append(buf_c2sId);
    ////channelcnt
    char buf_channelCnt[50] = {0};
    snprintf(buf_channelCnt,50,"%u",pSms->m_uChannelCnt);
    strCmd.append("  channelcnt ");
    strCmd.append(buf_channelCnt);
    ////submitdate
    char buf_submitDate[50] = {0};
    snprintf(buf_submitDate,50,"%ld",pSms->m_lSubmitDate);
    strCmd.append("  submitdate ");
    strCmd.append(buf_submitDate);

    //failed resend info
    LogDebug("[%s:%s],m_bResendCfgFlag [%d] ",pSms->m_strSmsId.data(),pSms->m_strPhone.data(),pSms->m_bResendCfgFlag);
    if(pSms->m_bResendCfgFlag && pSms->m_uDivideNum < 2)
    {
        smsTxSetRedisResendInfo(ss,strCmd);
    }
    pMsg->m_iMsgType = MSGTYPE_REDIS_REQ;
    pMsg->m_strKey= strkey;
    pMsg->m_RedisCmd = strCmd;
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,pMsg);
    LogNotice("[%s:%s] send msg to RedisThread strCmd[%s].",
             pSms->m_strSmsId.data(),pSms->m_strPhone.data(),strCmd.data());

    TRedisReq* pExpire = new TRedisReq();
    pExpire->m_iMsgType = MSGTYPE_REDIS_REQ;
    pExpire->m_strKey = strkey;
    pExpire->m_RedisCmd.assign("EXPIRE ");
    pExpire->m_RedisCmd.append(strkey);
    pExpire->m_RedisCmd.append("  259200");
    SelectRedisThreadPoolIndex(g_pRedisThreadPool, pExpire );

    /*  如果是长短信需要设置状态报告,
        长短信回来时用于匹配第一条记录, 只推送一条记录
        只有index = 1  并且是长短信的写redis
    */
    if( reportStatus->m_iIndexNum == 1 )
    {
        if( pSms->m_uDivideNum > 1 )
        {
            smsTxSetLongMsgRedis( pSms->m_strSmsId, pSms->m_strPhone );
        }
        smsTxSetMoRedisInfo( ss );
    }

    return CC_TX_RET_SUCCESS;
}

string CChannelThreadPool::smsTxGeneratRecordSqlCmd( smsp::SmsContext* pSms, CSMSTxStatusReport *reportStatus )
{
    char sql[10240]  = {0};
    struct tm timenow = {0};
    time_t now = 0;

    char dateTime[64] = {0};
    char submitTime[64] = {0};
    char subretTime[64] = {0};
    UInt32 uFailedResendFlag = 0;
    if ( pSms->m_lSendDate != 0 )
    {
        now = pSms->m_lSendDate;
    }
    else
    {
        now = time(NULL);
    }

    /*发送时间*/
    localtime_r( &now, &timenow );
    strftime(dateTime,sizeof(dateTime),"%Y%m%d%H%M%S",&timenow);

    string strRecordDate = pSms->m_strC2sTime.substr(0,8);
    if ( reportStatus->m_lSubmitDate!= 0 )
    {
        struct tm pTime = {0};
        localtime_r((time_t*)&reportStatus->m_lSubmitDate,&pTime);
        strftime(submitTime,sizeof(submitTime),"%Y%m%d%H%M%S",&pTime);
    }
    else
    {
        strftime(submitTime,sizeof(submitTime),"%Y%m%d%H%M%S",&timenow);
    }

    if ( reportStatus->m_lSubretDate != 0)
    {
        struct tm pTime = {0};
        localtime_r((time_t*)&reportStatus->m_lSubretDate,&pTime);
        strftime(subretTime,sizeof(subretTime),"%Y%m%d%H%M%S",&pTime);
    }
    else
    {
        strftime(subretTime,sizeof(subretTime),"%Y%m%d%H%M%S",&timenow);
    }

    char contentSQL[2500] = { 0 };
    char strSubret[1024] = {0};
    char strSign[512] = {0};
    char strTempParam[2048] = {0};
    MYSQL* MysqlConn = g_pDisPathDBThreadPool->CDBGetConn();
    if( MysqlConn != NULL )
    {
        mysql_real_escape_string(MysqlConn, contentSQL, reportStatus->m_strContent.data(),
                                 reportStatus->m_strContent.length());
        mysql_real_escape_string(MysqlConn, strSubret, reportStatus->m_strSubRet.data(),
                                 reportStatus->m_strSubRet.length());
        mysql_real_escape_string(MysqlConn, strSign, pSms->m_strSign.data(),
                                 pSms->m_strSign.length());
        if (!pSms->m_strTemplateParam.empty())
        {
            mysql_real_escape_string(MysqlConn, strTempParam, pSms->m_strTemplateParam.data(),
                                 pSms->m_strTemplateParam.length());
        }
    }

    char strBelongBusiness[32] = {0};
    snprintf( strBelongBusiness, sizeof(strBelongBusiness)-1, "%ld", pSms->m_uBelong_business);

    char cBelongSale[32] = {0};
    snprintf(cBelongSale,32,"%ld",pSms->m_uBelong_sale);
    string strBelongSale = cBelongSale;
    if((pSms->m_uResendTimes > 0) && !pSms->m_strFailedChannel.empty())
    {
        uFailedResendFlag = 1;
    }
    snprintf(sql,10240,"insert into t_sms_record_%d_%s(smsuuid,channelid,channeloperatorstype,channelremark,operatorstype,"
        "area,smsid,username,content,smscnt,state,phone,smsfrom,smsindex,costFee,channelsmsid,paytype,clientid,"
        "date,submit,submitdate,subret,subretdate,showphone,send_id,longsms,clientcnt,channelcnt,senddate,template_id,"
        "channel_tempid,temp_params,belong_sale, belong_business, sign, smstype, failed_resend_flag)  "
        " values('%s','%d','%d','%s','%d','%d','%s','%s','%s','%d','%d','%s','%d','%d','%f','%s','%d',"
        "'%s','%s','%s','%s','%s','%s','%s','%lu','%d','%d','%d','%s', %s, '%s', '%s', %s,%s,'%s', '%d','%u');",
        pSms->m_uChannelIdentify,strRecordDate.data(),
        reportStatus->m_strSmsUuid.data(),
        pSms->m_iChannelId,
        pSms->m_uChannelOperatorsType,
        pSms->m_strChannelRemark.data(),
        pSms->m_iOperatorstype,
        pSms->m_iArea,
        pSms->m_strSmsId.data(),
        pSms->m_strUserName.data(),
        contentSQL,
        pSms->m_iSmscnt,
        reportStatus->m_status,
        pSms->m_strPhone.substr(0, 20).data(),
        pSms->m_iSmsfrom,
        reportStatus->m_iIndexNum,
        (pSms->m_fCostFee)*1000,
        reportStatus->m_strChannelSmsid.data(),
        pSms->m_uPayType,
        pSms->m_strClientId.data(),
        pSms->m_strC2sTime.data(),
        reportStatus->m_strSubmit.data(),
        submitTime,
        strSubret,
        subretTime,
        pSms->m_strShowPhone.substr(0, 20).data(),
        g_uComponentId,
        pSms->m_ulongsms,
        pSms->m_uClientCnt,
        pSms->m_uChannelCnt,
        dateTime,
        (true == pSms->m_strTemplateId.empty()) ? "NULL" : pSms->m_strTemplateId.data(),
        pSms->m_strChannelTemplateId.data(),
        strTempParam,
        (!strBelongSale.compare("0")) ? "NULL" : strBelongSale.data(),
        ( 0 == pSms->m_uBelong_business ) ? "NULL" : strBelongBusiness,
        ( true == pSms->m_strSign.empty()) ? "NULL" : strSign,
        pSms->m_uSmsType,
        uFailedResendFlag);

    MONITOR_INIT( MONITOR_CHANNEL_SMS_SUBMIT );
    MONITOR_VAL_SET("clientid", pSms->m_strClientId );
    MONITOR_VAL_SET_INT("channelid", pSms->m_iChannelId );
    MONITOR_VAL_SET_INT("state", pSms->m_iStatus );
    MONITOR_VAL_SET("channelname", pSms->m_strChannelname );
    MONITOR_VAL_SET_INT("channeloperatorstype", pSms->m_uChannelOperatorsType );
    MONITOR_VAL_SET_INT("operatorstype", pSms->m_iOperatorstype );
    MONITOR_VAL_SET_INT("area", pSms->m_iArea );
    MONITOR_VAL_SET("username", pSms->m_strUserName );
    MONITOR_VAL_SET_INT("smsfrom",  pSms->m_iSmsfrom );
    MONITOR_VAL_SET_INT("paytype",  pSms->m_uPayType );
    MONITOR_VAL_SET("sign",     pSms->m_strSign  );
    MONITOR_VAL_SET_INT("smstype", pSms->m_uSmsType  );
    MONITOR_VAL_SET("date", pSms->m_strC2sTime );
    string strTableName = "t_sms_record_";
    strTableName.append( Comm::int2str( pSms->m_uChannelIdentify ));
    strTableName.append( "_" + strRecordDate );
    MONITOR_VAL_SET("dbtable", strTableName );
    MONITOR_VAL_SET("smsid", pSms->m_strSmsId );
    MONITOR_VAL_SET("phone", pSms->m_strPhone );
    MONITOR_VAL_SET("smsuuid", pSms->m_strSmsUuid );
    MONITOR_VAL_SET("submitcode", reportStatus->m_strYZXErrCode );
    MONITOR_VAL_SET("submitdesc", reportStatus->m_strSubmit );
    MONITOR_VAL_SET("submitdate", submitTime );
    MONITOR_VAL_SET("subretcode", strSubret );
    MONITOR_VAL_SET("subretdesc", reportStatus->m_strSubRet );
    MONITOR_VAL_SET("subretdate", subretTime );
    MONITOR_VAL_SET("synctime",   Comm::getCurrentTime_z(0));
    MONITOR_VAL_SET_INT("costtime",   time(NULL) - now );
    MONITOR_VAL_SET_INT("component_id", g_uComponentId );
    MONITOR_PUBLISH( g_pMQMonitorPublishThread );

    return string(sql);

}

UInt32 CChannelThreadPool::smsTxRecordInsertDb( smsp::SmsContext* pSms, CSMSTxStatusReport *reportStatus )
{
    if ( NULL == pSms || NULL == reportStatus )
    {
        LogError("******pSms is NULL.******");
        return CC_TX_RET_PARAM_ERROR;
    }

    string sql = smsTxGeneratRecordSqlCmd( pSms, reportStatus ) ;

    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    pMQ->m_strExchange.assign(g_strMQDBExchange );
    pMQ->m_strRoutingKey.assign(g_strMQDBRoutingKey );
    pMQ->m_strData.assign(sql);
    pMQ->m_strData.append("RabbitMQFlag=");
    pMQ->m_strData.append( reportStatus->m_strSmsUuid );
    g_pRecordMQProducerThread->PostMsg(pMQ);

    return CC_TX_RET_SUCCESS;

}


/* 获取流速开关*/
UInt32 CChannelThreadPool::smsTxFlowCtrSendReq( UInt32 uChannelId, UInt32 uLinkSize, Int32 iCount )
{
    channelTxFlowCtr *txFlowCtr = NULL;
    time_t now = time(NULL);

    map< int, models::Channel >::iterator itr =  m_ChannelMap.find( uChannelId );
    if( itr == m_ChannelMap.end())
    {
        LogWarn("TxFlowReq Channel[%u] Not Find", uChannelId );
        return CC_TX_RET_CHANNEL_NOT_EXSIT;
    }

    channelTxFlowCtr_t::iterator flowItr = m_mapChannelFlowCtr.find( uChannelId );
    if( flowItr == m_mapChannelFlowCtr.end())
    {
        txFlowCtr = new channelTxFlowCtr();
        txFlowCtr->lLastReqTime = now;
        txFlowCtr->iWindowSize  = itr->second.m_uSliderWindow * uLinkSize;
        m_mapChannelFlowCtr[ uChannelId ] = txFlowCtr;
    }
    else
    {
        txFlowCtr = flowItr->second;
    }

    if( NULL == txFlowCtr ){
        LogError("FlowCtr Param Errror txFlowCtr==NULL");
        return CC_TX_RET_PARAM_ERROR;
    }

    if ( itr->second.m_uClusterType != SMS_CLUSTER_TYPE_NONE )
    {
        iCount = 1;
    }

    txFlowCtr->uTotalReqCnt += iCount;
    if( txFlowCtr->uTotalReqCnt > 0xFFFFFFFFFFFFFFFE ){
        LogFatal("FlowCtr(%u) Reset SendReqTotal 0 ", uChannelId );
        txFlowCtr->uTotalReqCnt = 0;
    }

    if ( itr->second.m_uSliderWindow != 0 ){
        txFlowCtr->iWindowSize -= iCount;
    }

    txFlowCtr->lLastReqTime = now;

    if(txFlowCtr->iWindowSize <= 0)
    {
        txFlowCtr->uLastWndZeroTime = now;
    }

    LogNotice("FlowCtr(%u):[ uWindSize:[%d:%u], totalReqCnt:%u, totalRspCnt:%u, RetCnt:%d, ReqTimeElapse:%ld]",
            uChannelId, txFlowCtr->iWindowSize, itr->second.m_uSliderWindow * uLinkSize,
            txFlowCtr->uTotalReqCnt,txFlowCtr->uTotalRspCnt,
            txFlowCtr->uResetCount,
            now - txFlowCtr->lLastReqTime);

    txFlowCtr->lLastReqTime = now;

    return CC_TX_RET_SUCCESS;
}

/*流速响应回来*/
UInt32 CChannelThreadPool::smsTxFlowCtrSendRsp( UInt32 uChannelId, UInt32 uStatus, bool bFirstIndex, Int32 uCount  )
{
    channelTxFlowCtr *txFlowCtr = NULL;
    time_t now = time(NULL);

    map<int, models::Channel>::iterator itr =  m_ChannelMap.find( uChannelId );
    if( itr  == m_ChannelMap.end() )
    {
        LogWarn("TxFlowReq Channel[%u] Not Find", uChannelId );
        return CC_TX_RET_CHANNEL_NOT_EXSIT;
    }

    UInt32 uLinkSize = 1;
    if(itr->second.httpmode > HTTP_POST)
    {
        uLinkSize = GetWorkChannelNodeNum(uChannelId);
    }

    channelTxFlowCtr_t::iterator flowItr = m_mapChannelFlowCtr.find( uChannelId );
    if( flowItr == m_mapChannelFlowCtr.end())
    {
        LogWarn( "Channel[%u] not Exsit", uChannelId );
        return CC_TX_RET_CHANNEL_NOT_EXSIT;
    }

    txFlowCtr = flowItr->second;
    if( txFlowCtr->uTotalRspCnt > 0xFFFFFFFFFFFFFFFE ){
        LogFatal("FlowCtr(%u) Reset SendReqTotal 0 ", uChannelId );
        txFlowCtr->uTotalRspCnt = 0;
    }

    if ( itr->second.m_uClusterType != SMS_CLUSTER_TYPE_NONE ){
        uCount = 1;
    }

    /* 收到请求数增加*/
    txFlowCtr->uTotalRspCnt += uCount ;

    if( bFirstIndex != true )
    {
        /*发送侧如果有拆包，需要补齐数据*/
        txFlowCtr->uTotalReqCnt ++;
        if( uStatus == CC_TX_STATUS_REBACK ||
            uStatus == CC_TX_STATUS_TIMEOUT )
        {
            txFlowCtr->iWindowSize = CChanelTxFlowCtr::channelFlowCtrCountSub( FLOW_CTR_CALLER_POOL, uChannelId,
                            itr->second.m_uSliderWindow * uLinkSize, uCount );
        }
    }
    else
    {
        if( uStatus == CC_TX_STATUS_SUCCESS
            || uStatus == CC_TX_STATUS_FAIL
            || uStatus == CC_TX_STATUS_SOCKET_ERR_TO_REBACK)
        {
            txFlowCtr->iWindowSize = CChanelTxFlowCtr::channelFlowCtrCountAdd( uChannelId, itr->second.m_uSliderWindow * uLinkSize, uCount );
            txFlowCtr->lLastRspTime = now;
        }
    }

    /* 慢恢复滑窗机制*/
    PoolChannelFlowCtrSSRecover( uChannelId, now, itr->second.m_uSliderWindow * uLinkSize );

    /* 请求响应回来重置计数器*/
    if( uStatus == CC_TX_STATUS_SUCCESS
            || uStatus == CC_TX_STATUS_FAIL )
    {
        txFlowCtr->uResetCount = 0;
    }

    if(txFlowCtr->iWindowSize <= 0)
    {
        txFlowCtr->uLastWndZeroTime = now;
    }

    LogNotice("FlowCtr(%u):[ uWindSize:[%d:%u], totalReqCnt:%u, totalRspCnt:%u, RetCnt:%d, ReqTimeElapse:%ld, bFirstIndex:%d], uStatus[%u]",
            uChannelId, txFlowCtr->iWindowSize, itr->second.m_uSliderWindow * uLinkSize,
            txFlowCtr->uTotalReqCnt,txFlowCtr->uTotalRspCnt,
            txFlowCtr->uResetCount,
            now - txFlowCtr->lLastReqTime,
            bFirstIndex,
            uStatus);

    return CC_TX_RET_SUCCESS;

}

UInt32 CChannelThreadPool::smsTxFlowCtrCheckAndReset( )
{
    UInt32  reSetCnt = 0;
    time_t now = time( NULL );
    channelTxFlowCtr *txFlowCtr = NULL;

    channelTxFlowCtr_t::iterator flowItr = m_mapChannelFlowCtr.begin();
    while( flowItr != m_mapChannelFlowCtr.end())
    {
        txFlowCtr = flowItr->second;
        /* 如果已经达到最大滑窗，
            并且超过了滑窗最大等待时间，
            重置滑窗*/
        map<int, models::Channel>::iterator itr =  m_ChannelMap.find( flowItr->first );
        if( itr == m_ChannelMap.end())
        {
            LogWarn("TxFlowReq Channel %u Not Find", flowItr->first );
            SAFE_DELETE(txFlowCtr);
            m_mapChannelFlowCtr.erase(flowItr++);
            continue;
        }

        if( itr->second.httpmode > HTTP_POST
            || itr->second.m_uSliderWindow == 0 )
        {
            flowItr++;
            continue;//这里只针对http协议
        }

        UInt32 uCheckTime = itr->second.httpmode <= HTTP_POST ? CC_TX_CHANNEL_WINDOWSIZE_HTTP_RESET_TIME : CC_TX_CHANNEL_WINDOWSIZE_TCP_REST_TIME;

        if( txFlowCtr->iWindowSize <= 0 && now - txFlowCtr->lLastRspTime > uCheckTime )
        {
            /*滑窗重置三次、需要告警*/
            if( ++txFlowCtr->uResetCount >= CC_TX_CHANNEL_FLOWCTR_RESET_COUNT )
            {
                string strChinese = DAlarm::getNoSubmitRespAlarmStr( flowItr->first, itr->second.industrytype, uCheckTime * txFlowCtr->uResetCount );
                string strChanneId = Comm::int2str( flowItr->first );
                Alarm(CHNN_SLIDE_WINDOW_ALARM, strChanneId.data(), strChinese.data());
                LogWarn("FlowCtr(%u): Reset sliderWindow:%d retCnt %d Alarm...",
                        flowItr->first,itr->second.m_uSliderWindow, txFlowCtr->uResetCount );
                txFlowCtr->uResetCount = 0;
            }

            LogWarn("FlowCtr(%u): Reset sliderWindow:%d retCnt:%u ",
                    flowItr->first,itr->second.m_uSliderWindow, txFlowCtr->uResetCount );

            CChanelTxFlowCtr::channelFlowCtrSynchronous( flowItr->first, itr->second.m_uSliderWindow );
            txFlowCtr->iWindowSize = itr->second.m_uSliderWindow;
            txFlowCtr->lLastRspTime = now;
            reSetCnt++;

        }

        flowItr++;

    }

    return reSetCnt;

}


UInt32 CChannelThreadPool::smsTxCountSubmitFailed( UInt32 uChannelId, bool bResult )
{
    channelMap_t::iterator iter = m_ChannelMap.find(uChannelId);
    if(iter == m_ChannelMap.end())
    {
        LogWarn("Channel[%u] not found in m_ChannelMap", uChannelId);
        return CC_TX_RET_CHANNEL_NOT_EXSIT;
    }

    channelTxFlowCtr *txFlowCtr = NULL;
    channelTxFlowCtr_t::iterator flowItr = m_mapChannelFlowCtr.find( uChannelId );
    if( flowItr == m_mapChannelFlowCtr.end())
    {
        LogWarn("Channel[%u] not Exsit", uChannelId);
        return CC_TX_RET_CHANNEL_NOT_EXSIT;
    }

    txFlowCtr = flowItr->second;

    if ( true == bResult )
    {
        txFlowCtr->uSubmitFailedCnt = 0;
        return CC_TX_RET_SUCCESS ;
    }

    txFlowCtr->uSsErrorNum++;

    if ( ++txFlowCtr->uSubmitFailedCnt >= m_uSubmitFailedValue )
    {
        char temp[15] = {0};
        snprintf(temp,15,"%u",uChannelId);
        LogWarn("channelid[%u] continue submit failed num[%d] is over systemValue[%d]",
                uChannelId, txFlowCtr->uSubmitFailedCnt, m_uSubmitFailedValue );
        std::string strChinese = "";
        strChinese = DAlarm::getAlarmStr(uChannelId, iter->second.industrytype, txFlowCtr->uSubmitFailedCnt, m_uSubmitFailedValue);

        Alarm(CHNN_SUBMIT_NO_RESPONSE_ALARM,temp,strChinese.data());
        txFlowCtr->uSubmitFailedCnt = 0;
    }

    return CC_TX_RET_SUCCESS;
}


UInt32 CChannelThreadPool::smsTxCountSendFailed( UInt32 uChannelId, bool bResult )
{
    channelMap_t::iterator iter = m_ChannelMap.find(uChannelId);
    if(iter == m_ChannelMap.end())
    {
        LogWarn("Channel[%u] not found in m_ChannelMap", uChannelId);
        return CC_TX_RET_CHANNEL_NOT_EXSIT;
    }

    channelTxFlowCtr *txFlowCtr = NULL;
    channelTxFlowCtr_t::iterator flowItr = m_mapChannelFlowCtr.find( uChannelId );
    if( flowItr == m_mapChannelFlowCtr.end())
    {
        LogWarn("ChannelId[%u] not Exsit", uChannelId);
        return CC_TX_RET_CHANNEL_NOT_EXSIT;
    }

    txFlowCtr = flowItr->second;

    if (true == bResult)
    {
        txFlowCtr->uSendFailedCnt  = 0;
        return CC_TX_RET_SUCCESS;
    }

    txFlowCtr->uSsErrorNum++;

    if ( ++txFlowCtr->uSendFailedCnt >= m_uSendFailedValue )
    {
        char temp[15] = {0};
        snprintf(temp,15,"%u",uChannelId);
        LogWarn("channelid[%u] continue send failed num[%lu] is over systemValue[%u]",
                uChannelId,txFlowCtr->uSendFailedCnt ,m_uSendFailedValue );
        std::string strChinese = "";
        strChinese = DAlarm::getAlarmStr1(uChannelId, iter->second.industrytype, txFlowCtr->uSendFailedCnt, m_uSubmitFailedValue);
        Alarm(CHNN_FAILURE_ALARM,temp,strChinese.data());
        txFlowCtr->uSendFailedCnt = 0;
    }

    return CC_TX_RET_SUCCESS;

}



/*获取当前处理会话数量*/
UInt32 CChannelThreadPool::GetSessionMapSize()
{
    return m_mapSessions.size();
}

UInt32 CChannelThreadPool::smsTxGetSystemErrorCode(string strError,string strSubmit,string& strErrorOut,string& strSubmitOut)
{

    pthread_rwlock_rdlock( &m_rwlock  );

    map<string,string>::iterator itrError = m_mapSystemErrorCode.find(strError);
    if (itrError == m_mapSystemErrorCode.end())
    {
        strErrorOut.assign(strError);
    }
    else
    {
        strErrorOut.assign(strError);
        strErrorOut.append("*");
        strErrorOut.append(itrError->second);
    }

    map<string,string>::iterator itrSub = m_mapSystemErrorCode.find(strSubmit);
    if (itrSub == m_mapSystemErrorCode.end())
    {
        strSubmitOut.assign(strSubmit);
    }
    else
    {
        strSubmitOut.assign(strSubmit);
        strSubmitOut.append("*");
        strSubmitOut.append(itrSub->second);
    }

    pthread_rwlock_unlock( &m_rwlock );

    return CC_TX_RET_SUCCESS;

}


/*需要合并错误码*/
void CChannelThreadPool::smsTxDirectGetErrorCode(string strDircet, UInt32 uChannelId,
                                   string strType,string strIn, string& strDesc, string& strReportDesc )
{
    string strKey1 = "";
    strKey1.assign(strDircet);
    strKey1.append("_");
    strKey1.append(strType);
    strKey1.append("_");
    strKey1.append(strIn);

    map<string,channelErrorDesc>::iterator itrDesc1 = m_mapChannelErrorCode.find(strKey1);
    /*如果协议错误不存在,使用通道错误码*/
    if (itrDesc1 == m_mapChannelErrorCode.end())
    {
        string strKey2 = "";
        strKey2.assign(Comm::int2str(uChannelId));
        strKey2.append("_");
        strKey2.append(strType);
        strKey2.append("_");
        strKey2.append(strIn);
        /*通道错误码*/
        map<string,channelErrorDesc>::iterator itrDesc2 = m_mapChannelErrorCode.find(strKey2);
        if (itrDesc2 == m_mapChannelErrorCode.end())
        {
            string strUrl = "*%e6%9c%aa%e7%9f%a5";
            strUrl = http::UrlCode::UrlDecode(strUrl);

            strDesc.assign(strIn);
            strDesc.append(strUrl);
            strReportDesc.assign(strIn);
        }
        else
        {
            if (true == itrDesc2->second.m_strSysCode.empty())
            {
                strDesc.assign(strIn);
                strDesc.append("*");
                strDesc.append(itrDesc2->second.m_strErrorDesc);

                strReportDesc.assign(strIn);
            }
            else
            {
                strDesc.assign(strIn);
                strDesc.append("*");
                strDesc.append(itrDesc2->second.m_strErrorDesc);

                strReportDesc.assign(itrDesc2->second.m_strSysCode);
            }
        }
    }
    else
    {
        if (true == itrDesc1->second.m_strSysCode.empty())
        {
            strDesc.assign(strIn);
            strDesc.append("*");
            strDesc.append(itrDesc1->second.m_strErrorDesc);

            strReportDesc.assign(strIn);
        }
        else
        {
            strDesc.assign(strIn);
            strDesc.append("*");
            strDesc.append(itrDesc1->second.m_strErrorDesc);

            strReportDesc.assign(itrDesc1->second.m_strSysCode);
        }
    }

    return;
}

void CChannelThreadPool::smsTxDirectGetInnerError(string strDircet, UInt32 uChannelId,
                                  string strType,string strIn, string& strInnerError)
{
   string strKey1 = "";
   strKey1.assign(strDircet);
   strKey1.append("_");
   strKey1.append(strType);
   strKey1.append("_");
   strKey1.append(strIn);

   map<string,channelErrorDesc>::iterator itrDesc1 = m_mapChannelErrorCode.find(strKey1);

   if (itrDesc1 == m_mapChannelErrorCode.end())
   {
       string strKey2 = "";
       strKey2.assign(Comm::int2str(uChannelId));
       strKey2.append("_");
       strKey2.append(strType);
       strKey2.append("_");
       strKey2.append(strIn);
       /*通道错误码*/
       map<string,channelErrorDesc>::iterator itrDesc2 = m_mapChannelErrorCode.find(strKey2);
       if (itrDesc2 == m_mapChannelErrorCode.end())
       {
           string strUrl = "*%e6%9c%aa%e7%9f%a5";
           strUrl = http::UrlCode::UrlDecode(strUrl);

           strInnerError.assign(strIn);
           strInnerError.append(strUrl);
       }
       else
       {
           if (itrDesc2->second.m_strInnnerCode.empty())
           {
                strInnerError.assign(strIn);
                strInnerError.append("*");
                strInnerError.append(itrDesc2->second.m_strErrorDesc);
           }
           else
           {
                strInnerError.assign(itrDesc2->second.m_strInnnerCode);
           }

       }
   }
   else
   {
       if (itrDesc1->second.m_strInnnerCode.empty())
       {
            strInnerError.assign(strIn);
            strInnerError.append("*");
            strInnerError.append(itrDesc1->second.m_strErrorDesc);
       }
       else
       {
            strInnerError.assign(itrDesc1->second.m_strInnnerCode);
       }
   }

   return;
}


/* 获取错误码*/
UInt32 CChannelThreadPool::smsTxGetErrorCode(string strFlag, string strIn, string& strDesc,string& strReportDesc)
{
    string strKey = "";
    strKey.append(strFlag);
    strKey.append("_1_");
    strKey.append(strIn);

    pthread_rwlock_rdlock( &m_rwlock  );

    map<string,channelErrorDesc>::iterator itrDesc = m_mapChannelErrorCode.find(strKey);
    if (itrDesc != m_mapChannelErrorCode.end())
    {
        if (true == itrDesc->second.m_strSysCode.empty()) ////syscode kong
        {
            strDesc.assign(itrDesc->second.m_strErrorCode);
            strDesc.append("*");
            strDesc.append(itrDesc->second.m_strErrorDesc);
            strReportDesc.assign(strIn);
        }
        else
        {
            strDesc.assign(itrDesc->second.m_strErrorCode);
            strDesc.append("*");
            strDesc.append(itrDesc->second.m_strErrorDesc);

            strReportDesc.assign(itrDesc->second.m_strSysCode);
        }
    }
    else
    {
        string strUrl = "*%e6%9c%aa%e7%9f%a5";
        strUrl = http::UrlCode::UrlDecode(strUrl);
        strDesc.assign(strIn);
        strDesc.append(strUrl);
        strReportDesc.assign(strIn);
    }

    pthread_rwlock_unlock( &m_rwlock );

    return CC_TX_RET_SUCCESS;

}

void CChannelThreadPool::PoolChannelFlowCtrDelete(UInt32 uChannelId)
{
    /*删除共享流控数据*/
    CChanelTxFlowCtr::channelFlowCtrDelete( uChannelId );

    /*删除通道流控缓存数据*/
    channelTxFlowCtr_t::iterator flowItr = m_mapChannelFlowCtr.find( uChannelId );
    if( flowItr != m_mapChannelFlowCtr.end())
    {
        SAFE_DELETE(flowItr->second);
        m_mapChannelFlowCtr.erase(flowItr);
    }
}

void  CChannelThreadPool::PoolChannelFlowCtrNew(UInt32 uChannelId, UInt32 uWindowSize, bool bCluster )
{
    channelTxFlowCtr *txFlowCtr = new channelTxFlowCtr();
    txFlowCtr->lLastReqTime = time(NULL);
    txFlowCtr->iWindowSize  = uWindowSize;
    txFlowCtr->lLastErrTime = txFlowCtr->lLastReqTime;
    txFlowCtr->uSsErrorNum  = 0;
    txFlowCtr->uSsthresh = uWindowSize/2 + 1;

    m_mapChannelFlowCtr[uChannelId] = txFlowCtr;

    /*添加共享流控数据*/
    CChanelTxFlowCtr::channelFlowCtrNew( uChannelId, uWindowSize, uWindowSize == 0, bCluster );

    LogNotice("Channel[%u] WndSize[%u]", uChannelId, uWindowSize);
}

void CChannelThreadPool::PoolChannelFlowCtrReset(UInt32 uChannelId, int uWindowSize, bool bCluster )
{
    //更新滑窗
    channelTxFlowCtr_t::iterator flowItr = m_mapChannelFlowCtr.find( uChannelId );
    if( flowItr != m_mapChannelFlowCtr.end())
    {
        flowItr->second->iWindowSize = uWindowSize;
        flowItr->second->uResetCount = 0;
        flowItr->second->lLastErrTime   = time(NULL);
        flowItr->second->uSsErrorNum    = 0;
        flowItr->second->uSsthresh = uWindowSize/2 + 1;
        CChanelTxFlowCtr::channelFlowCtrSetSize( uChannelId, flowItr->second->iWindowSize, bCluster );
    }

    LogNotice("Channel[%u] iWindowSize[%d]", uChannelId, flowItr->second->iWindowSize);

}

void CChannelThreadPool::PoolChannelFlowCtrModify(UInt32 uChannelId, int iWnd)
{
    //更新滑窗
    channelTxFlowCtr_t::iterator flowItr = m_mapChannelFlowCtr.find( uChannelId );
    if( flowItr != m_mapChannelFlowCtr.end())
    {
        flowItr->second->iWindowSize += iWnd;

        if(flowItr->second->iWindowSize < 0)
        {
            flowItr->second->iWindowSize = 0;
            flowItr->second->uLastWndZeroTime = time(NULL);
        }

        LogDebug("Channel[%u] modify[%d] CurrWnd[%d]", uChannelId, iWnd, flowItr->second->iWindowSize);
        //全局滑窗与pool滑窗同步
        CChanelTxFlowCtr::channelFlowCtrSynchronous( uChannelId, flowItr->second->iWindowSize);
    }
}

void CChannelThreadPool::PoolChannelFlowCtrModify(UInt32 uChannelId, int iWnd, int iMaxWnd)
{
    //更新滑窗
    channelTxFlowCtr_t::iterator flowItr = m_mapChannelFlowCtr.find( uChannelId );
    if( flowItr != m_mapChannelFlowCtr.end())
    {
        flowItr->second->iWindowSize += iWnd;

        if(flowItr->second->iWindowSize < 0)
        {
            flowItr->second->iWindowSize = 0;
            flowItr->second->uLastWndZeroTime = time(NULL);
        }

        if(flowItr->second->iWindowSize > iMaxWnd)
        {
            flowItr->second->iWindowSize = iMaxWnd;
        }

        LogDebug("Channel[%u] modify[%d] MaxWnd[%d] CurrWnd[%d]", uChannelId, iWnd, iMaxWnd, flowItr->second->iWindowSize);
        //全局滑窗与pool滑窗同步
        CChanelTxFlowCtr::channelFlowCtrSynchronous( uChannelId, flowItr->second->iWindowSize);
    }
}

/*
* 滑窗恢复机制在3 秒的时间窗口内，系统没有收到失败的响应，
  判定当前滑窗应该恢复处理，如果滑窗已经达到恢复阀值不做处理。
*/
UInt32 CChannelThreadPool::PoolChannelFlowCtrSSRecover( UInt32 uChannelId, Int64 timeNow, UInt32 uMaxWindowSize )
{
    channelTxFlowCtr_t::iterator flowItr = m_mapChannelFlowCtr.find( uChannelId );
    if( flowItr == m_mapChannelFlowCtr.end()){
        return 0;
    }

    if ( flowItr->second->lLastErrTime == 0  ||
        ( timeNow - flowItr->second->lLastErrTime < SS_TIME_WINDOW_SECS ))
    {
        return 0;
    }

    flowItr->second->lLastErrTime = timeNow;

    if ( flowItr->second->uSsErrorNum == 0 )
    {
        if ( flowItr->second->iWindowSize < flowItr->second->uSsthresh )
        {
            Int32 iWindowsSize = flowItr->second->iWindowSize;
            if ( iWindowsSize < 0 ){
                flowItr->second->iWindowSize =  iWindowsSize/2 + 1;
            }else{
                flowItr->second->iWindowSize =  iWindowsSize * 2;
            }

            if( flowItr->second->iWindowSize > (Int32)uMaxWindowSize ){
                flowItr->second->iWindowSize = (Int32)uMaxWindowSize;
            }
            CChanelTxFlowCtr::channelFlowCtrSynchronous( uChannelId, flowItr->second->iWindowSize );
            LogNotice("FlowCtr(%u),uSSthresh:%d, windowSize[%d --> %d]",
                      uChannelId, flowItr->second->uSsthresh,
                      iWindowsSize, flowItr->second->iWindowSize );
        }
        else
        {
            /*恢复滑窗恢复  阀值*/
            flowItr->second->uSsthresh *=  2;
            if ( flowItr->second->uSsthresh > uMaxWindowSize ){
                flowItr->second->uSsthresh = uMaxWindowSize;
            }
        }
    }
    else
    {
        /* 减小慢恢复阀值*/
        flowItr->second->uSsthresh = flowItr->second->uSsthresh/2 + 1;
    }

    flowItr->second->uSsErrorNum = 0;

    LogDebug("FlowCtr(%d), curWindowSize:%d:%d, uSsthresh:%u ",
             uChannelId, flowItr->second->iWindowSize,
             uMaxWindowSize,
             flowItr->second->uSsthresh );

    return flowItr->second->iWindowSize;

}

void CChannelThreadPool::smsTxUpdateSgipChannel ( TMsg* pMsg )
{
    TUpdateChannelReq *pNewChannelMsg = (TUpdateChannelReq *)pMsg;

    m_ChannelSgipMap.clear();
    m_ChannelSgipMap = pNewChannelMsg->m_ChannelMap;
    LogNotice("smsTxUpdateSgipChannel update,size[%d] ", m_ChannelSgipMap.size())
}

/*通道更新*/
UInt32 CChannelThreadPool::smsTxUpdateChannel ( TMsg* pMsg )
{
    TUpdateChannelReq *pNewChannelMsg = (TUpdateChannelReq *)pMsg;

    std::map<int, models::Channel> mapOldChannel = m_ChannelMap;
    m_ChannelMap.clear();
    m_ChannelMap = pNewChannelMsg->m_ChannelMap;

    std::map<int, models::Channel> mapHttpChannelDel;
    std::map<int, models::Channel> mapHttpChannelAdd;
    std::map<int, models::Channel> mapDirectChannelDel;
    std::map<int, models::Channel> mapDirectChannelAdd;
    std::map<int, models::Channel> mapChannelWindowUpdate;

    mapHttpChannelDel.clear();
    mapHttpChannelAdd.clear();
    mapDirectChannelDel.clear();
    mapDirectChannelAdd.clear();
    mapChannelWindowUpdate.clear();

    /* 获取删除的通道 */
    for ( std::map<int,models::Channel>::iterator ito = mapOldChannel.begin(); ito != mapOldChannel.end(); ito++ )
    {
        map< int, models::Channel>::iterator itn = pNewChannelMsg->m_ChannelMap.find( ito->first );
        if ( itn == pNewChannelMsg->m_ChannelMap.end())//需要删除这条通道
        {
            LogWarn("Channel[%d] is Closed.", ito->first);

            if(ito->second.httpmode <= HTTP_POST)
            {
                mapHttpChannelDel[ito->first] = ito->second;
            }
            else
            {
                mapDirectChannelDel[ito->first] = ito->second;
            }
        }
    }

    /* 获取新增的通道*/
    for ( std::map<int,models::Channel>::iterator itrn = pNewChannelMsg->m_ChannelMap.begin(); itrn != pNewChannelMsg->m_ChannelMap.end(); itrn++ )
    {
        map< int, models::Channel>::iterator itro = mapOldChannel.find( itrn->first );
        if ( itro == mapOldChannel.end())
        {
            LogWarn("Add Channel[%d].", itrn->first );
            if(itrn->second.httpmode <= HTTP_POST)
            {
                mapHttpChannelAdd[itrn->first] = itrn->second;
            }
            else
            {
                mapDirectChannelAdd[itrn->first] = itrn->second;
            }
        }
        else
        {
            if (   (itrn->second.httpmode != itro->second.httpmode)                 //协议类型
                || (itrn->second.m_uSendNodeId != itro->second.m_uSendNodeId)       //send结点
                || (itrn->second.m_uProtocolType != itro->second.m_uProtocolType)   //子协议类型
                || (itrn->second.m_uLinkNodenum != itro->second.m_uLinkNodenum)     //连接数
                || IsChannelUpdate(itro->second, itrn->second)
            )
            {
                if(itrn->second.m_uSendNodeId != itro->second.m_uSendNodeId)
                {
                    //改变了通道挂载的send结点
                    LogWarn("Channel[%d]'s send node id has changed from [%d] to [%d]!",
                        itrn->first, itro->second.m_uSendNodeId, itrn->second.m_uSendNodeId);

                    if(itro->second.httpmode <= HTTP_POST)
                    {
                        mapHttpChannelDel[itro->first] = itro->second;
                    }
                    else
                    {
                        mapDirectChannelDel[itro->first] = itro->second;
                    }
                }
                else
                {
                    //协议改变
                    if(itro->second.httpmode != itrn->second.httpmode)
                    {
                        //直连协议和http协议互相改
                        if( (itro->second.httpmode <= HTTP_POST && itrn->second.httpmode >= YD_CMPP) ||
                            (itro->second.httpmode > HTTP_POST && itrn->second.httpmode < YD_CMPP)
                          )
                        {
                            if(itro->second.httpmode <= HTTP_POST)//http改直连
                            {
                                mapHttpChannelDel[itro->first] = itro->second;  //删除老的http协议
                                mapDirectChannelAdd[itrn->first] = itrn->second;//添加新的直连协议
                            }
                            else//直连协议改http协议
                            {
                                mapDirectChannelDel[itro->first] = itro->second;//删除老的直连协议
                                mapHttpChannelAdd[itrn->first] = itrn->second;  //添加新的http协议
                            }

                            //PoolChannelFlowCtrDelete(itro->first);
                        }
                        else if(itro->second.httpmode > HTTP_POST && itrn->second.httpmode > HTTP_POST)//直连改直连
                        {
                            mapDirectChannelDel[itro->first] = itro->second;//删除老的直连协议
                            mapDirectChannelAdd[itrn->first] = itrn->second;//添加新的直连协议

                            //PoolChannelFlowCtrDelete(itro->first);
                        }
                        else
                        {
                            ;
                        }
                    }
                    else
                    {
                        if(itrn->second.httpmode == YD_CMPP && itro->second.m_uProtocolType != itrn->second.m_uProtocolType)
                        {
                            mapDirectChannelDel[itro->first] = itro->second;//删除老的直连协议
                            mapDirectChannelAdd[itrn->first] = itrn->second;//添加新的直连协议
                        }

                        if(itrn->second.m_uLinkNodenum != itro->second.m_uLinkNodenum)
                        {
                            LogDebug("Channel[%d] LinkNodenum from %d to %d", itro->first, itro->second.m_uLinkNodenum, itrn->second.m_uLinkNodenum);
                            if( itrn->second.m_uLinkNodenum != itro->second.m_uLinkNodenum )
                            {
                                //适配改变了结点数
                                AdaptChannelNodeNumChanged(itro->first, itrn->second.m_uLinkNodenum, itro->second.m_uSliderWindow, itrn->second.m_uSliderWindow);
                            }
                        }

                        if(IsChannelUpdate(itro->second, itrn->second))
                        {
                            if ( (itro->second.m_uExValue & CHANNEL_OWN_SPLIT_FLAG) != (itrn->second.m_uExValue & CHANNEL_OWN_SPLIT_FLAG) )
                            {
                                if (itro->second.httpmode <= HTTP_POST)
                                {
                                    mapHttpChannelDel[itro->first] = itro->second;
                                    mapHttpChannelAdd[itrn->first] = itrn->second;
                                }
                                else
                                {
                                    mapDirectChannelDel[itro->first] = itro->second;//???é?¤è€??????′è?????è??
                                    mapDirectChannelAdd[itrn->first] = itrn->second;//?·?????–°?????′è?????è??
                                }
                            }
                            else
                            {
                                UpdateChannelReq(itrn->first);

                                if( ( itro->second.m_uSliderWindow != itrn->second.m_uSliderWindow
                                      && itrn->second.m_uLinkNodenum == itro->second.m_uLinkNodenum
                                      && itrn->second.httpmode > HTTP_POST
                                    )
                                    ||
                                    ((itro->second.m_uSliderWindow != itrn->second.m_uSliderWindow
                                      || itro->second.m_uClusterType !=  itrn->second.m_uClusterType )
                                     && itrn->second.httpmode <= HTTP_POST
                                    )
                                   )
                                    {
                                        mapChannelWindowUpdate[itrn->first] = itrn->second;
                                    }
                            }

                        }
                    }
                }
            }
        }
    }

    LogNotice("mapHttpChannelDel.Size[%d], mapHttpChannelAdd.size[%d], mapDirectChannelDel.size[%d]"
       ", mapDirectChannelAdd.size[%d], mapChannelWindowUpdate.size[%d]",
        mapHttpChannelDel.size(), mapHttpChannelAdd.size(), mapDirectChannelDel.size(),
        mapDirectChannelAdd.size(), mapChannelWindowUpdate.size());

    /* 通知处理线程删除*/
    if( mapHttpChannelDel.size() > 0 )
    {
        /*发送通道删除消息到http处理线程*/
        for(channeThreads_t::iterator txThItr  = m_poolTxThreads.begin(); txThItr != m_poolTxThreads.end(); txThItr++ )
        {
            TUpdateChannelReq* phttpChannelDel = new TUpdateChannelReq();
            phttpChannelDel->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ;
            phttpChannelDel->m_ChannelMap = mapHttpChannelDel;
            (*txThItr)->PostMsg( phttpChannelDel );
        }

        /*删除通道与线程对应关系*/
        for( map< int, models::Channel>::iterator itr = mapHttpChannelDel.begin();
              itr != mapHttpChannelDel.end(); itr++ )
        {
            channeThreadMap_t::iterator chanThreadItr = m_mapChanneThread.find(itr->first);
            if( chanThreadItr != m_mapChanneThread.end())
            {
                m_mapChanneThread.erase( chanThreadItr );
            }

            PoolChannelFlowCtrDelete(itr->first);
        }
    }

    //为删除的直连协议清理处理线程
    for( map< int, models::Channel>::iterator itr = mapDirectChannelDel.begin(); itr != mapDirectChannelDel.end(); itr++ )
    {
        channeThreadMap_t::iterator chanThreadItr = m_mapChanneThread.find(itr->first);
        if( chanThreadItr != m_mapChanneThread.end())
        {
            //这条通道绑定了这几个线程，销毁他们跟通道的连接
            for(channeThreads_t::iterator it = chanThreadItr->second.m_vecThreads.begin(); it != chanThreadItr->second.m_vecThreads.end(); it++)
            {
                TDirectChannelUpdateMsg *delMsg = new TDirectChannelUpdateMsg();
                delMsg->m_iChannelId = itr->first;
                delMsg->m_iMsgType   = MSGTYPE_RULELOAD_CHANNEL_DEL_REQ;
                delMsg->m_lDelTime   = time(NULL);
                delMsg->m_Channel    = itr->second;
                delMsg->m_bDelAll    = true;
                static_cast<CDirectSendThread*>(*it)->PostMsg(delMsg);
            }

            //清除保存的关系
            m_mapChanneThread.erase( chanThreadItr );
            LogNotice("m_mapChanneThread delete Channel[%d]", itr->first);

            //删除通道登录状态
            if(m_mapChannelLoginState.find(itr->first) != m_mapChannelLoginState.end())
            {
                m_mapChannelLoginState.erase(itr->first);
            }

            //删除滑窗
            PoolChannelFlowCtrDelete(itr->first);
        }
    }

    /* 增加通道处理将新增加的通道
        分配到不同的处理线程*/
    for( map< int, models::Channel>::iterator itr = mapHttpChannelAdd.begin(); itr != mapHttpChannelAdd.end(); itr++ )
    {
        channeThreadMap_t::iterator chanThreadItr = m_mapChanneThread.find(itr->first);
        if(chanThreadItr == m_mapChanneThread.end())
        {

            channelThreadsData_t threadData;
            threadData.m_vecThreads = m_poolTxThreads;
            m_mapChanneThread[itr->first] = threadData;  /*默认发送到所有线程*/
        }
        PoolChannelFlowCtrNew( itr->first, itr->second.m_uSliderWindow, itr->second.m_uClusterType != 0 );
    }

    //为新增的直连协议初始化处理线程
    for( map< int, models::Channel>::iterator itr = mapDirectChannelAdd.begin(); itr != mapDirectChannelAdd.end(); itr++ )
    {
        channeThreadMap_t::iterator chanThreadItr = m_mapChanneThread.find(itr->first);

        if(chanThreadItr == m_mapChanneThread.end())
        {
            channelThreadsData_t threadData;
            UInt32               maxConnect = 1;

            if( itr->second.m_uLinkNodenum < m_poolTxDirectThreads.size())
            {
                maxConnect = itr->second.m_uLinkNodenum;
            }
            else
            {
                maxConnect = m_poolTxDirectThreads.size();
            }

            InitDirectChannelThreads(maxConnect, itr->first);

            InitChannelThreadInfo(itr->first);
        }
    }

    for( map< int, models::Channel>::iterator itr = mapChannelWindowUpdate.begin(); itr != mapChannelWindowUpdate.end(); itr++ )
    {
        if(itr->second.httpmode > HTTP_POST)
        {
            PoolChannelFlowCtrReset(itr->first, GetWorkChannelNodeNum(itr->first)*itr->second.m_uSliderWindow);
        }
        else
        {
            PoolChannelFlowCtrReset(itr->first, itr->second.m_uSliderWindow, itr->second.m_uClusterType != 0 );
        }
    }

    return CC_TX_RET_SUCCESS;
}

bool  CChannelThreadPool::IsChannelUpdate(models::Channel oldChannel, models::Channel newChannel)
{
    return (   (oldChannel._accessID != newChannel._accessID)
            || (oldChannel.m_uExtendSize != newChannel.m_uExtendSize)
            || (oldChannel.m_strServerId != newChannel.m_strServerId)
            || (oldChannel.m_uChannelIdentify != newChannel.m_uChannelIdentify)
            || (oldChannel.m_uExValue != newChannel.m_uExValue)
            || (oldChannel.m_uChannelType != newChannel.m_uChannelType)
            || (oldChannel.url != newChannel.url)
            || (oldChannel._clientID != newChannel._clientID)
            || (oldChannel._password != newChannel._password)
            || (oldChannel.channelname != newChannel.channelname)
            || (oldChannel.m_uSliderWindow != newChannel.m_uSliderWindow)
            || (oldChannel.spID != newChannel.spID)
            || (oldChannel.m_uClusterType != newChannel.m_uClusterType )
            || (oldChannel.m_uSplitRule != newChannel.m_uSplitRule )
            || (oldChannel.m_uCnLen != newChannel.m_uCnLen )
            || (oldChannel.m_uEnLen != newChannel.m_uEnLen )
            || (oldChannel.m_uCnSplitLen != newChannel.m_uCnSplitLen )
            || (oldChannel.m_uEnSplitLen != newChannel.m_uEnSplitLen ));
}

/* 检查是否需要进入reback */
UInt32 CChannelThreadPool::smsTxCheckReback( channelTxSessions *session, CSMSTxStatusReport *reportStatus )
{
    UInt32 status = CC_TX_STATUS_INIT;

    smsp::SmsContext  *pSms = session->m_context;

    /* 处理状态计数器*/
    if( reportStatus->m_strSubmit == SOCKET_IS_ERROR
        || reportStatus->m_strSubmit == SOCKET_IS_CLOSE
        || reportStatus->m_strSubmit == HTTPSEND_INIT_FAILED )
    {
        smsTxCountSubmitFailed( pSms->m_iChannelId, false );
        status = CC_TX_STATUS_REBACK;
    }
    else
    {
        smsTxCountSendFailed( pSms->m_iChannelId, false );

        if( reportStatus->m_strSubmit == RESPONSE_IS_NOT_200 ){
            smsTxCountSubmitFailed(pSms->m_iChannelId, true );
        }

        if( reportStatus->m_strSubmit == LIB_IS_NULL ){
            status = CC_TX_STATUS_REBACK;
        }else{
            status = CC_TX_STATUS_FAIL;
        }
    }

    if( status == CC_TX_STATUS_REBACK )
    {
        /* 如果先前已经reback, 不处理,
           如果有分段错误则报错*/
        if( session->m_uStatus == CC_TX_STATUS_REBACK )
        {
            status = CC_TX_STATUS_DISCARD;
        }
        else
        {
            if( session->m_uErrCnt > 0 )
            {
                status = CC_TX_STATUS_FAIL;
            }
        }
    }

    return status;
}

UInt32 CChannelThreadPool::smsTxStausToCluster( UInt32 ClusterStatus, channelTxSessions *session,
                              CSMSTxStatusReport *statusInfo, string strExchange, string strRoutingKey )
{
    if(NULL == session || NULL == statusInfo ){
        LogError("Session Process Err: Param Null");
        return CC_TX_RET_PARAM_ERROR;
    }

    if( session->m_pSender )
    {
        CSMSClusterStatusReqMsg *pMsg = new CSMSClusterStatusReqMsg();
        pMsg->m_iMsgType = MSGTYPE_CHANNEL_SEND_STATUS_REPORT;
        pMsg->m_iSeq = session->m_iSeq;
        pMsg->m_strSessionID = Comm::int2str( session->m_uClusterSession );
        pMsg->txClusterStatus.txReport = *statusInfo;
        pMsg->txClusterStatus.strExchange = strExchange;
        pMsg->txClusterStatus.strRoutingKey = strRoutingKey;
        pMsg->txClusterStatus.uMsgType = ClusterStatus;
        session->m_pSender->PostMsg( pMsg );
    }else{
        LogWarn("Cluster Send Error");
        return CC_TX_RET_PARAM_ERROR;
    }

    return CC_TX_RET_SUCCESS;

}

/*更新状态*/
UInt32 CChannelThreadPool::smsTxChannelSendStatusReport( TMsg* pMsg )
{
    UInt32 ret = CC_TX_RET_SUCCESS;
    UInt32 status = CC_TX_STATUS_INIT;

    CSMSTxStatusReportMsg* reportMsg = (CSMSTxStatusReportMsg *)pMsg;

    CSMSTxStatusReport reportStatus = reportMsg->report;

    channeSessionMap_t::iterator ssItr = m_mapSessions.find( reportMsg->m_strSessionID );
    if( ssItr == m_mapSessions.end())
    {
        LogWarn("Session %s Not Find ", reportMsg->m_strSessionID.c_str());
        return CC_TX_RET_SESSION_NOT_FIND;
    }

    channelTxSessions *session = ssItr->second;
    smsp::SmsContext *pSms = session->m_context;

    pSms->m_iStatus   = reportStatus.m_status;
    pSms->m_strSubmit = reportStatus.m_strSubmit;
    pSms->m_strYZXErrCode = reportStatus.m_strYZXErrCode;
    pSms->m_strSubret = reportStatus.m_strSubRet;
    pSms->m_strChannelSmsId = reportStatus.m_strChannelSmsid ;
    pSms->m_uChannelCnt = reportStatus.m_uChanCnt; /*拆分条数*/
    pSms->m_lSubmitDate = reportStatus.m_lSubmitDate;
    pSms->m_lSubretDate = reportStatus.m_lSubretDate;
    pSms->m_uDivideNum  = reportStatus.m_uTotalCount; /*发送条数*/
    pSms->m_iSmscnt     = reportStatus.m_iSmsCnt; /*计费条数*/
    pSms->m_ulongsms    = reportStatus.m_uChanCnt> 1 ? 1 : 0 ;

    /* 重新合成uuid, 保证uudi唯一性*/
    reportStatus.m_strSmsUuid =  pSms->m_strSmsUuid.substr(0, 10 ) + reportStatus.m_strSmsUuid;
    session->m_uTotalCnt= reportStatus.m_uTotalCount;
    session->m_uRecvCnt++;

    /*状态分发*/
    switch( reportStatus.m_status )
    {
        case SMS_STATUS_SOCKET_ERR_TO_REBACK:
        {
            status = CC_TX_STATUS_SOCKET_ERR_TO_REBACK;
            ret = smsTxSendToReBack( session, &reportStatus );
            break;
        }
        case SMS_STATUS_CONFIRM_FAIL:
        case SMS_STATUS_SEND_FAIL:
        case SMS_STATUS_SUBMIT_FAIL:
        {
            status = smsTxCheckReback( session, &reportStatus );

            if( CC_TX_STATUS_REBACK == status )
            {
                ret = smsTxSendToReBack( session, &reportStatus );
            }
            else if( CC_TX_STATUS_FAIL == status )
            {
                ret = smsTxSendFailReport( session, &reportStatus );
            }
            break;
        }

        case SMS_STATUS_SEND_SUCCESS:
        case SMS_STATUS_SUBMIT_SUCCESS:
        case SMS_STATUS_CONFIRM_SUCCESS:
        {
            if( reportStatus.m_strSubmit == TIME_OUT
                || reportStatus.m_strSubmit == CMPP_REPLY_TIME_OUT
                || reportStatus.m_strSubmit == SGIP_REPLY_TIME_OUT
                || reportStatus.m_strSubmit == SMGP_REPLY_TIME_OUT
                || reportStatus.m_strSubmit == SMPP_REPLY_TIME_OUT)
            {
                status = CC_TX_STATUS_TIMEOUT;
            }
            else
            {
                status = CC_TX_STATUS_SUCCESS;
            }
            ret = smsTxSendSuccReport( session, &reportStatus );
            break;
        }
        default:
            ;;
    }

    LogDebug( "SessionId:%s, SmsId:%s, Phone:%s, RebackStaus:%d",
               reportMsg->m_strSessionID.c_str(),
               pSms->m_strSmsId.c_str(),
               pSms->m_strPhone.c_str(),
               status );

    smsTxFlowCtrSendRsp( pSms->m_iChannelId, status, session->m_uRecvCnt == 1, session->m_uPhoneCnt );

    /*已经接受所有的分段删除会话*/
    if( session->m_uRecvCnt == session->m_uTotalCnt )
    {
        LogNotice("TxSessionID:%s, Phone:%s Delete",
                  reportMsg->m_strSessionID.c_str(), pSms->m_strPhone.c_str());
        SESSION_FREE( session );
        m_mapSessions.erase( ssItr );
    }
    else
    {   /*重置定时器处理分段*/
        SAFE_DELETE(session->m_pWheelTime);
        session->m_pWheelTime = SetTimer( CC_TX_CHANNLE_SESSION_TIMER_SEG_ID,
                     pSms->m_strSmsUuid, CC_TX_CHANNEL_SESSION_TIMEOUT_30S );
    }

    return  ret;
}

/*
    for set failed resend info to redis
    add by ljl 2018-01-20

*/
void CChannelThreadPool::smsTxSetRedisResendInfo( channelTxSessions* pSession, string& strRedisData )
{
    if(!pSession || !pSession->m_context)
    {
        LogWarn("pSms is null!!");
        return;
    }
    smsp::SmsContext *pSms = pSession->m_context;
    UInt32 uProcessTimes = 0;
    string strErrorDate = "";

    if(pSession->m_bDirectProt)
    {
        smsDirectInfo *pSmsDirect = (smsDirectInfo *)pSession->m_reqMsg;
        uProcessTimes = pSmsDirect->m_uProcessTimes;
        strErrorDate  = pSmsDirect->m_strErrorDate;
    }
    else
    {
        SmsHttpInfo *pSmsHttp = ( SmsHttpInfo * )pSession->m_reqMsg;
        uProcessTimes = pSmsHttp->m_uProcessTimes;
        strErrorDate  = pSmsHttp->m_strErrorDate;
    }

    //smstype
    strRedisData.append("  smstype ");
    strRedisData.append(Comm::int2str(pSms->m_uSmsType));

    if(pSms->m_uResendTimes > 0 && !pSms->m_strFailedChannel.empty())
    {
        //failed_resend_times
        strRedisData.append("  failed_resend_times ");
        strRedisData.append(Comm::int2str(pSms->m_uResendTimes));
        //failed_resend_channel
        strRedisData.append("  failed_resend_channel ");
        strRedisData.append(pSms->m_strFailedChannel);
    }
    //failed_resend_msg
    string strData;
    ////userName
    strData.assign("userName=");
    strData.append(Base64::Encode(pSms->m_strUserName));
    ////sign
    strData.append("&sign=");
    strData.append(Base64::Encode(pSms->m_strSign));
    ////content
    strData.append("&content=");
    strData.append(Base64::Encode(pSms->m_strContent));
    ////smsType
    strData.append("&smsType=");
    strData.append(Comm::int2str( pSms->m_uSmsType));
    ////paytype
    strData.append("&paytype=");
    strData.append(Comm::int2str(pSms->m_uPayType));
    ////showsigntype
    strData.append("&showsigntype=");
    strData.append(pSms->m_strShowSignType_c2s);
    ////userextpendport
    strData.append("&userextpendport=");
    strData.append(pSms->m_strUserExtendPort_c2s);

    ////signextendport
    strData.append("&signextendport=");
    strData.append(pSms->m_strSignExtendPort_c2s);

    ////operater
    strData.append("&operater=");
    strData.append(Comm::int2str(pSms->m_iOperatorstype));
    ////ids
    strData.append("&ids=");
    strData.append(pSms->m_strAccessids);
    ////clientcnt
    strData.append("&clientcnt=");
    strData.append(Comm::int2str(pSms->m_uClientCnt));
    ////process_times
    strData.append("&process_times=");
    strData.append(Comm::int2str(uProcessTimes));
    ////csid
    strData.append("&csid=");
    strData.append(Comm::int2str(pSms->m_uC2sId));
    ////csdate
    strData.append("&csdate=");
    strData.append(pSms->m_strC2sTime);
    ////errordate
    strData.append("&errordate=");
    if ( 1 == uProcessTimes )
    {
        UInt64 uNowTime = time(NULL);
        strData.append(Comm::int2str(uNowTime));
    }
    else
    {
        strData.append(strErrorDate);
    }
    strData.append("&templateid=");
    strData.append(pSms->m_strTemplateId);
    strData.append("&temp_params=");
    strData.append(pSms->m_strTemplateParam);

    strRedisData.append(" failed_resend_msg ");
    strRedisData.append(strData);
    return;
}

/*
    check DB cfg is support failed resend
    1.sys switch
    2.account FailedResendFlag
    3.channel ResendErrCode
    4.SmsType resendtimes and is timeout
*/
template<typename T>
bool CChannelThreadPool::smsTxCheckFailedResendCfg(T& smsInfo)
{
    if(false == m_bResendSysSwitch)
    {
        LogDebug("clientid[%s] sys resend switch is not open!",smsInfo.m_strClientId.c_str());
        return false;
    }

    std::map<std::string,SmsAccount>::iterator iterAccount = m_accountMap.find(smsInfo.m_strClientId);
    if( (iterAccount != m_accountMap.end()) && (1 == iterAccount->second.m_uFailedResendFlag) )
    {
        //continue
    }
    else
    {
        LogDebug("clientid[%s] switch is not open or not in m_accountMap!", smsInfo.m_strClientId.c_str());
        return false;
    }

    channelMap_t::iterator iterChannel = m_ChannelMap.find(smsInfo.m_iChannelId);
    if(iterChannel != m_ChannelMap.end() && !iterChannel->second.m_strResendErrCode.empty())
    {
        //continue
    }
    else
    {
        LogError("[%s:%s:%s:%d] ResendErrcode is null or channel not in ChannelMap",
            smsInfo.m_strClientId.c_str(), smsInfo.m_strSmsId.c_str()
            , smsInfo.m_strPhone.c_str(), smsInfo.m_iChannelId);
        return false;
    }

    time_t nowTime = time(NULL);
    time_t c2sTime = Comm::getTimeStampFormDate(smsInfo.m_strC2sTime,"%Y%m%d%H%M%S");
    if(-1 == c2sTime)
    {
        LogError("get c2stime is error");
        return false;
    }
    std::map<UInt32,FailedResendSysCfg>::iterator iterSysCfg = m_FailedResendSysCfgMap.find(smsInfo.m_uSmsType);
    if((iterSysCfg != m_FailedResendSysCfgMap.end()) &&
        (0 < iterSysCfg->second.m_uResendMaxTimes) &&
        ((nowTime - c2sTime) < iterSysCfg->second.m_uResendMaxDuration))
    {
        //contine
    }
    else
    {
        if(iterSysCfg != m_FailedResendSysCfgMap.end())
            LogNotice("[%s:%s:%s:%d],smstype[%u] resend cfg is not paas ResendMaxTimes[%u],ResendMaxDuration[%u]"
                , smsInfo.m_strClientId.c_str(), smsInfo.m_strSmsId.c_str()
                , smsInfo.m_strPhone.c_str(), smsInfo.m_iChannelId
                , smsInfo.m_uSmsType,iterSysCfg->second.m_uResendMaxTimes,iterSysCfg->second.m_uResendMaxDuration);
        return false;
    }
    return true;
}
