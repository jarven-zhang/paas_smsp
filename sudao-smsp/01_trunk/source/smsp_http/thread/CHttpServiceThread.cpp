#include "CHttpServiceThread.h"
#include "UrlCode.h"
#include "HttpParams.h"
#include "http/httprequest.h"
#include "http/httpresponse.h"
#include "json/json.h"
#include "Comm.h"
#include "main.h"
#include "UTFString.h"
#include "Uuid.h"
#include "compress.h"
#include "base64.h"
#include "BusTypes.h"
#include "ops_cpusched.h"
#include "global.h"

#include "boost/algorithm/string.hpp"

#include <time.h>
#include <stdlib.h>


const int HTTPS_RESPONSE_CODE_0 = 0;
const int HTTPS_RESPONSE_CODE_1 = -1;
const int HTTPS_RESPONSE_CODE_2 = -2;
const int HTTPS_RESPONSE_CODE_3 = -3;
const int HTTPS_RESPONSE_CODE_4 = -4;
const int HTTPS_RESPONSE_CODE_5 = -5;
const int HTTPS_RESPONSE_CODE_6 = -6;
const int HTTPS_RESPONSE_CODE_7 = -7;
const int HTTPS_RESPONSE_CODE_8 = -8;
const int HTTPS_RESPONSE_CODE_9 = -9;
const int HTTPS_RESPONSE_CODE_10 = -10;
const int HTTPS_RESPONSE_CODE_11 = -11;
const int HTTPS_RESPONSE_CODE_12 = -12;
const int HTTPS_RESPONSE_CODE_13 = -13;
const int HTTPS_RESPONSE_CODE_14 = -14;
const int HTTPS_RESPONSE_CODE_15 = -15;
const int HTTPS_RESPONSE_CODE_16 = -16;
const int HTTPS_RESPONSE_CODE_17 = -17;
const int HTTPS_RESPONSE_CODE_18 = -18;
const int HTTPS_RESPONSE_CODE_19 = -19;
const int HTTPS_RESPONSE_CODE_20 = -20;
const int HTTPS_RESPONSE_CODE_21 = -21;
const int HTTPS_RESPONSE_CODE_22 = -22;
const int HTTPS_RESPONSE_CODE_23 = -23;
const int HTTPS_RESPONSE_CODE_24 = -24;
const int HTTPS_RESPONSE_CODE_25 = -25;
const int HTTPS_RESPONSE_CODE_26 = -26;
const int HTTPS_RESPONSE_CODE_27 = -27;
const int HTTPS_RESPONSE_CODE_28 = -28;
const int HTTPS_RESPONSE_CODE_29 = -29;
const int HTTPS_RESPONSE_CODE_30 = -30;
const int HTTPS_RESPONSE_CODE_31 = -31;
const int HTTPS_RESPONSE_CODE_33 = -33;
const int HTTPS_RESPONSE_CODE_34 = -34;
const int HTTPS_RESPONSE_CODE_35 = -35;
const int HTTPS_RESPONSE_CODE_36 = -36;
const int HTTPS_RESPONSE_CODE_37 = -37;
const int HTTPS_RESPONSE_CODE_38 = -38; // 无效的extend参数值

const unsigned int TEMPLATE_PARAMETER_MAX_LEN = 1000;

const  int ML_MAX_ERR = 128;
const static std::string ML_error_reply[ML_MAX_ERR] =
{
    "",
    "error:",
    "error:Username missing or error",
    "error:Password missing or error",
    "error:Missing apikey",
    "error:Missing recipient",
    "error:Missing message content",
    "error:Account is blocked",
    "error:Unrecognized encoding",
    "error:APIKEY or password error",
    "error:Unauthorized IP address",
    "error:Account balance is insufficient",
    "error:Throughput Rate Exceeded",
    "error:Invalid md5 password length",
    "error:Smstype config is empty",
    "error:Account is frozen",
    "error:Account is canceled",
    "error:Protocal error",
    "error:Content too long",
    "error:Missing sign",
    "error:Sign too short",
    "error:Sign too long",
    "error:Sign unknown",
    "error:Phone too many",
    "error:Phone formate error",
    "error:Phone foreign not",
    "error:Phone black",
    "error:Phone repeate"
};

const char *Transfer2JDErrCode(int err)
{

    switch(err)
    {
    case HTTPS_RESPONSE_CODE_0:
    {

        return "0";
    }
    case HTTPS_RESPONSE_CODE_1:  		//鉴权失败（帐号或密码错误）
    {

        return RESP_CODE_JD_AUTHENTICATION_ERR;
    }
    case HTTPS_RESPONSE_CODE_2:  		//账号余额不足
    {

        return RESP_CODE_JD_INSUFFICIENT_BALANCE;
    }
    case HTTPS_RESPONSE_CODE_3:  		//账号被注销
    {

        return RESP_CODE_JD_EXPIRED_ACCOUNT;
    }
    case HTTPS_RESPONSE_CODE_4:  		//账号被锁定
    {

        return RESP_CODE_JD_EXPIRED_ACCOUNT;
    }
    case HTTPS_RESPONSE_CODE_5:  		//ip鉴权失败
    {

        return RESP_CODE_JD_PERMISSION_DENY;
    }
    case HTTPS_RESPONSE_CODE_6:  		//发送号码为空
    {

        return RESP_CODE_JD_INVALID_MOBILE;
    }
    case HTTPS_RESPONSE_CODE_7:  		//手机号码格式错误
    {

        return RESP_CODE_JD_INVALID_MOBILELIST;
    }
    case HTTPS_RESPONSE_CODE_8:  		//短信内容超长
    {

        return RESP_CODE_JD_MSG_OVERSIZE;
    }
    case HTTPS_RESPONSE_CODE_9:  		//签名未报备
    {

        return RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_10:  		//协议类型不匹配
    {

        return RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_11:  		//不允许拉取状态报告
    {

        return RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_12:  		//访问频率过快
    {

        return RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_13:  		//您的费用信息不存在
    {

        return RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_14:  		//内部错误
    {

        return RESP_CODE_JD_INTERNAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_15:  		//用户对应的模板ID不存在、或模板未通过审核、或模板已删除
    {

        return RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_16:  		//模板参数不匹配
    {

        return RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_17:  		//USSD、闪信和挂机短信模板不允许发送国际号码
    {

        return RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_18:  		//模板ID为空
    {

        return RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_19:  		//模板参数含有非法字符
    {

        return RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_20:  		//json格式错误
    {

        return RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_21:  		//解析json失败
    {

        return RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_22:  		//账号被冻结
    {
        return RESP_CODE_JD_EXPIRED_ACCOUNT;
    }
    case HTTPS_RESPONSE_CODE_23:  		//短信类型为空
    {

        return RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_24:  		//短信内容为空
    {

        return RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_25:  		//发送号码数量超过100个
    {

        return RESP_CODE_JD_INVALID_MOBILELIST;
    }
    case HTTPS_RESPONSE_CODE_26:  		//未找到签名
    {

        return RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_27:  		//签名长度过短（少于2个字）
    {

        return RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_28:  		//签名长度过长
    {

        return  RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_29:  		//发送号码黑名单
    {

        return RESP_CODE_JD_GENERAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_30:  		//重复的发送号码
    {

        return RESP_CODE_JD_INVALID_MOBILELIST;
    }
    default:
        return RESP_CODE_JD_GENERAL_ERR;
    }
}


std::string &CHttpServiceThread::trim(std::string &s)
{
    if (s.empty())
    {
        return s;
    }

    std::string::iterator c;
    // Erase whitespace before the string
    for (c = s.begin(); c != s.end() && iswspace(*c++);)
        ;
    s.erase(s.begin(), --c);

    // Erase whitespace after the string
    for (c = s.end(); c != s.begin() && iswspace(*--c);)
        ;
    s.erase(++c, s.end());

    return s;
}


CHttpServiceThread::CHttpServiceThread(const char *name):
    CThread(name)
{
    m_pInternalService = NULL;
    m_pSnManager = NULL;
    ////m_phoneDao = NULL;
}

CHttpServiceThread::~CHttpServiceThread()
{
	SAFE_DELETE(m_pSnManager);
}


bool CHttpServiceThread::Init(const std::string ip, unsigned int port)
{
    //urlToUtf();

    if (false == CThread::Init())
    {
        LogError("CThread::Init is failed.");
        return false;
    }

    m_mapAccount.clear();
    g_pRuleLoadThread->getSmsAccountMap(m_mapAccount);

    m_mapSmsTemplate.clear();
    g_pRuleLoadThread->getSmsTemplateMap(m_mapSmsTemplate);

    m_mapSystemErrorCode.clear();
    g_pRuleLoadThread->getSystemErrorCode(m_mapSystemErrorCode);

    m_AgentInfoMap.clear();
    g_pRuleLoadThread->getAgentInfo(m_AgentInfoMap);

    m_pSnManager = new SnManager();
    if(NULL == m_pSnManager)
    {
        LogError("new SnManager() is failed.");
        return false;
    }

    m_pInternalService = new InternalService();
    if(m_pInternalService == NULL)
    {
        LogError("m_pInternalService is NULL.");
        return false;
    }

    m_pInternalService->Init();
    m_pServerSocekt = m_pInternalService->CreateServerSocket(this);
    if (false == m_pServerSocekt->Listen(Address(ip, port)))
    {
        printf("m_pServerSocekt->Listen is failed.\n");
        return false;
    }

    m_pLinkedBlockPool = new LinkedBlockPool();

    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_0] =  http::UrlCode::UrlDecode(string("%e6%88%90%e5%8a%9f")); // 成功
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_1] =  http::UrlCode::UrlDecode(string("%e9%89%b4%e6%9d%83%e5%a4%b1%e8%b4%a5%ef%bc%88%e8%b4%a6%e5%8f%b7%e6%88%96%e5%af%86%e7%a0%81%e9%94%99%e8%af%af%ef%bc%89")); // 鉴权失败（账号或密码错误）
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_2] =  http::UrlCode::UrlDecode(string("%e8%b4%a6%e5%8f%b7%e4%bd%99%e9%a2%9d%e4%b8%8d%e8%b6%b3")); // 账号余额不足
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_3] =  http::UrlCode::UrlDecode(string("%e8%b4%a6%e5%8f%b7%e8%a2%ab%e6%b3%a8%e9%94%80")); // 账号被注销
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_4] =  http::UrlCode::UrlDecode(string("%e8%b4%a6%e5%8f%b7%e8%a2%ab%e9%94%81%e5%ae%9a")); // 账号被锁定
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_5] =  http::UrlCode::UrlDecode(string("ip%e9%89%b4%e6%9d%83%e5%a4%b1%e8%b4%a5")); // ip鉴权失败
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_6] =  http::UrlCode::UrlDecode(string("%e5%8f%91%e9%80%81%e5%8f%b7%e7%a0%81%e4%b8%ba%e7%a9%ba")); // 发送号码为空
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_7] =  http::UrlCode::UrlDecode(string("%e6%89%8b%e6%9c%ba%e5%8f%b7%e7%a0%81%e6%a0%bc%e5%bc%8f%e9%94%99%e8%af%af")); // 手机号码格式错误
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_8] =  http::UrlCode::UrlDecode(string("%e7%9f%ad%e4%bf%a1%e5%86%85%e5%ae%b9%e8%b6%85%e9%95%bf")); // 短信内容超长
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_9] =  http::UrlCode::UrlDecode(string("%e7%ad%be%e5%90%8d%e6%9c%aa%e6%8a%a5%e5%a4%87")); // 签名未报备
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_10] =  http::UrlCode::UrlDecode(string("%e5%8d%8f%e8%ae%ae%e7%b1%bb%e5%9e%8b%e4%b8%8d%e5%8c%b9%e9%85%8d")); // 协议类型不匹配
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_11] =  http::UrlCode::UrlDecode(string("%e4%b8%8d%e5%85%81%e8%ae%b8%e6%8b%89%e5%8f%96%e7%8a%b6%e6%80%81%e6%8a%a5%e5%91%8a")); // 不允许拉取状态报告
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_12] =  http::UrlCode::UrlDecode(string("%e8%ae%bf%e9%97%ae%e9%a2%91%e7%8e%87%e8%bf%87%e5%bf%ab")); // 访问频率过快
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_13] =  http::UrlCode::UrlDecode(string("%e6%82%a8%e7%9a%84%e8%b4%b9%e7%94%a8%e4%bf%a1%e6%81%af%e4%b8%8d%e5%ad%98%e5%9c%a8")); // 您的费用信息不存在
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_14] =  http::UrlCode::UrlDecode(string("%e5%86%85%e9%83%a8%e9%94%99%e8%af%af")); // 内部错误
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_15] =  http::UrlCode::UrlDecode(string("%e7%94%a8%e6%88%b7%e5%af%b9%e5%ba%94%e7%9a%84%e6%a8%a1%e6%9d%bfID%e4%b8%8d%e5%ad%98%e5%9c%a8%e3%80%81%e6%88%96%e6%a8%a1%e6%9d%bf%e6%9c%aa%e9%80%9a%e8%bf%87%e5%ae%a1%e6%a0%b8%e3%80%81%e6%88%96%e6%a8%a1%e6%9d%bf%e5%b7%b2%e5%88%a0%e9%99%a4")); // 用户对应的模板ID不存在、或模板未通过审核、或模板已删除
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_16] =  http::UrlCode::UrlDecode(string("%e6%a8%a1%e6%9d%bf%e5%8f%82%e6%95%b0%e4%b8%8d%e5%8c%b9%e9%85%8d")); // 模板参数不匹配
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_17] =  http::UrlCode::UrlDecode(string("USSD%e3%80%81%e9%97%aa%e4%bf%a1%e5%92%8c%e6%8c%82%e6%9c%ba%e7%9f%ad%e4%bf%a1%e6%a8%a1%e6%9d%bf%e4%b8%8d%e5%85%81%e8%ae%b8%e5%8f%91%e9%80%81%e5%9b%bd%e9%99%85%e5%8f%b7%e7%a0%81")); // USSD、闪信和挂机短信模板不允许发送国际号码
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_18] =  http::UrlCode::UrlDecode(string("%e6%a8%a1%e6%9d%bfID%e4%b8%ba%e7%a9%ba")); // 模板ID为空
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_19] =  http::UrlCode::UrlDecode(string("%e6%a8%a1%e6%9d%bf%e5%8f%82%e6%95%b0%e5%90%ab%e6%9c%89%e9%9d%9e%e6%b3%95%e5%ad%97%e7%ac%a6%e6%88%96%e8%80%85%e8%b6%85%e8%bf%87%e9%95%bf%e5%ba%a6%e9%99%90%e5%88%b6")); // 模板参数含有非法字符或者模板参数超过长度限制
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_20] =  http::UrlCode::UrlDecode(string("json%e6%a0%bc%e5%bc%8f%e9%94%99%e8%af%af")); // json格式错误
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_21] =  http::UrlCode::UrlDecode(string("%e8%a7%a3%e6%9e%90json%e5%a4%b1%e8%b4%a5")); // 解析json失败
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_22] =  http::UrlCode::UrlDecode(string("%e8%b4%a6%e5%8f%b7%e8%a2%ab%e5%86%bb%e7%bb%93")); // 账号被冻结
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_23] =  http::UrlCode::UrlDecode(string("%E7%9F%AD%E4%BF%A1%E7%B1%BB%E5%9E%8B%E4%B8%BA%E7%A9%BA%E6%88%96%E9%9D%9E%E5%AE%9A%E4%B9%89%E6%95%B0%E5%80%BC")); // 短信类型为空或非定义数值
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_24] =  http::UrlCode::UrlDecode(string("%e7%9f%ad%e4%bf%a1%e5%86%85%e5%ae%b9%e4%b8%ba%e7%a9%ba")); // "短信内容为空"
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_25] =  http::UrlCode::UrlDecode(string("%e5%8f%91%e9%80%81%e5%8f%b7%e7%a0%81%e6%95%b0%e9%87%8f%e8%b6%85%e8%bf%87%e5%8d%8f%e8%ae%ae%e8%a7%84%e5%ae%9a%e4%b8%aa%e6%95%b0"));
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_26] =  http::UrlCode::UrlDecode(string("%e6%9c%aa%e6%89%be%e5%88%b0%e7%ad%be%e5%90%8d")); // 未找到签名
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_27] =  http::UrlCode::UrlDecode(string("%e7%ad%be%e5%90%8d%e9%95%bf%e5%ba%a6%e8%bf%87%e7%9f%ad")); // 签名长度过短
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_28] =  http::UrlCode::UrlDecode(string("%e7%ad%be%e5%90%8d%e9%95%bf%e5%ba%a6%e8%bf%87%e9%95%bf")); // "签名长度过长"
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_29] =  http::UrlCode::UrlDecode(string("%e5%8f%91%e9%80%81%e5%8f%b7%e7%a0%81%e9%bb%91%e5%90%8d%e5%8d%95")); // "发送号码黑名单"
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_30] =  http::UrlCode::UrlDecode(string("%e7%94%b5%e8%af%9d%e5%8f%b7%e7%a0%81%e9%87%8d%e5%a4%8d")); // "电话号码重复"
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_31] =  http::UrlCode::UrlDecode(string("%e7%bc%96%e7%a0%81%e9%94%99%e8%af%af")); // "编码错误"
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_33] =  http::UrlCode::UrlDecode(string("%e5%ae%9a%e6%97%b6%e7%9f%ad%e4%bf%a1%e6%97%b6%e9%97%b4%e6%a0%bc%e5%bc%8f%e9%94%99%e8%af%af")); //定时短信时间格式错误
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_34] =  http::UrlCode::UrlDecode(string("%e5%ae%9a%e6%97%b6%e5%8f%91%e9%80%81%e6%97%b6%e9%97%b4%e5%a4%aa%e7%9f%ad")); //定时发送时间太短
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_35] =  http::UrlCode::UrlDecode(string("%e5%ae%a2%e6%88%b7%e4%b8%8d%e6%94%af%e6%8c%81%e5%8f%91%e9%80%81%e5%ae%9a%e6%97%b6%e7%9f%ad%e4%bf%a1")); //客户不支持发送定时短信
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_36] =  http::UrlCode::UrlDecode(string("%e5%8f%b7%e7%a0%81%e8%a7%a3%e5%8e%8b%e5%a4%b1%e8%b4%a5")); //号码解压失败
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_37] =  http::UrlCode::UrlDecode(string("uid%e9%95%bf%e5%ba%a6%e8%b6%85%e8%bf%8760")); //uid长度超过60
    m_mapHttpErrCodeDes[HTTPS_RESPONSE_CODE_38] =  http::UrlCode::UrlDecode(string("%E6%97%A0%E6%95%88%E7%9A%84extend%E5%8F%82%E6%95%B0%E5%80%BC")); //无效的extend参数值

    map<std::string, std::string> sysParamMap;
    g_pRuleLoadThread->getSysParamMap(sysParamMap);
    UpdateBySysPara(sysParamMap);

    return true;
}

void CHttpServiceThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    uint32_t 	core_id = 0;

    get_core_for_probeif(&core_id, 1);
    ops_setaffinity(core_id);
    nice(-20);

    while(true)
    {
        UInt32 iSelect = m_pInternalService->GetSelector()->Select();
        m_pTimerMng->Click();

        pthread_mutex_lock(&m_mutex);
        TMsg *pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if(pMsg == NULL && 0 == iSelect)
        {
            usleep(g_uSecSleep);
        }
        else if(pMsg != NULL)
        {
            HandleMsg(pMsg);
            SAFE_DELETE(pMsg);
        }
    }

    m_pInternalService->GetSelector()->Destroy();
}

void CHttpServiceThread::HandleMsg(TMsg *pMsg)
{
    CHK_NULL_RETURN(pMsg);

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    switch (pMsg->m_iMsgType)
    {
    case MSGTYPE_ACCEPTSOCKET_REQ:
    {
        HandleHTTPServiceAcceptSocketMsg(pMsg);
        break;
    }
    case MSGTYPE_SMSERVICE_REQ:
    case MSGTYPE_TENCENT_SMSERVICE_REQ:
    case MSGTYPE_TIMERSEND_SMSERVICE_REQ:
    {
        HandleHTTPServerReqMsg(pMsg);
        break;
    }
    case MSGTYPE_SMSERVICE_REQ_ML:
    {
        HandleHTTPServerReqMsg_ML(pMsg);
        break;
    }
    case MSGTYPE_SMSERVICE_REQ_2:
    {
        HandleHTTPServerReqMsg_2(pMsg);
        break;
    }
    case MSGTYPE_SMSERVICE_REQ_JD:			//for JD
    {
        HandleHTTPServerReqMsg_JD(pMsg);
        break;
    }
    case MSGTYPE_TEMPLATE_SMSERVICE_REQ:
    {
        HandleHTTPServerTemplateReqMsg(pMsg);
        break;
    }
    case MSGTYPE_UNITY_TO_HTTPS_RESP:
    {
        HandleUnityRespMsg(pMsg);
        break;
    }
    case MSGTYPE_SOCKET_WRITEOVER:
    {
        HandleHTTPServiceReturnOverMsg(pMsg);
        break;
    }
    case MSGTYPE_HTTP_SERVICE_RESP:		//for other thread return httpresponse
    {
        HandleHTTPResponse(pMsg->m_iSeq, pMsg->m_strSessionID);
        break;
    }
    case MSGTYPE_TIMEOUT:
    {
        LogWarn("timeout iSeq[%lu]!", pMsg->m_iSeq);
        if("ACCESS_HTTPSERVER_SESSION_TIMEOUT" == pMsg->m_strSessionID)
        {
            HandleHTTPServiceReturnOverMsg(pMsg);
        }
        break;
    }
    case MSGTYPE_RULELOAD_SMS_TEMPLATE_UPDATE_REQ:
    {
        TUpdateSmsTemplateReq *pSmsTemplateReq = (TUpdateSmsTemplateReq *)pMsg;
        if (pSmsTemplateReq)
        {
            m_mapSmsTemplate.clear();
            m_mapSmsTemplate = pSmsTemplateReq->m_mapSmsTemplate;
            LogNotice("RuleUpdate sms template update!");
        }
        break;
    }
    case MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ:
    {
        TUpdateSmsAccontReq *pAccountUpdateReq = (TUpdateSmsAccontReq *)pMsg;
        if (pAccountUpdateReq)
        {
            m_mapAccount.clear();
            m_mapAccount = pAccountUpdateReq->m_SmsAccountMap;
            LogNotice("RuleUpdate account update!");
        }
        break;
    }
    case MSGTYPE_RULELOAD_SYSTEM_ERROR_DESC_EEQ:
    {
        TUpdateSystemErrorCodeReq *pHttp = (TUpdateSystemErrorCodeReq *)pMsg;
        if (pHttp)
        {
            m_mapSystemErrorCode.clear();
            m_mapSystemErrorCode = pHttp->m_mapSystemErrorCode;
            LogNotice("update t_sms_system_error_desc size:%d.", m_mapSystemErrorCode.size());
        }
        break;
    }
    case MSGTYPE_RULELOAD_AGENTINFO_UPDATE_REQ:
    {
        TUpdateAgentInfoReq *pReq = (TUpdateAgentInfoReq *)pMsg;
        if (pReq)
        {
            m_AgentInfoMap.clear();
            m_AgentInfoMap = pReq->m_AgentInfoMap;
            LogNotice("update t_sms_agent_info update size[%d]", pReq->m_AgentInfoMap.size());
        }
        break;
    }
    case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
    {
        TUpdateSysParamRuleReq *pReq = (TUpdateSysParamRuleReq *)pMsg;
        if (pReq)
        {
            UpdateBySysPara(pReq->m_SysParamMap);
        }
        break;
    }
    default:
    {
        LogWarn("msgType:0x[%x] is invalid!", pMsg->m_iMsgType);
        break;
    }
    }
}

void CHttpServiceThread::UpdateBySysPara(const std::map<std::string, std::string> &mapSysPara)
{
    string strSysPara = "";
    std::map<std::string, std::string>::const_iterator iter;

    do
    {
        strSysPara = "TIMER_SEND_CFG";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string &strTmp = iter->second;
        std::vector<std::string> vCfg;
        Comm::splitExVectorSkipEmptyString(const_cast<std::string &>(strTmp), ";", vCfg);
        if (vCfg.size() < 6)
        {
            LogError("Invalid 'TIMER_SEND_CFG' value: [%s] -> reason:size<6", strTmp.c_str());
            break;
        }
        UInt32 uTmp = atoi(vCfg[4].c_str());

        if ((uTmp < 1) || (60 < uTmp))
        {
            LogError("Out of range of system parameter(%s) value(%s, %u).",
                     strSysPara.c_str(), strTmp.c_str(), uTmp);

            break;
        }
        m_uTimersendReqMinInterval = uTmp;
    }
    while (0);

    LogNotice("timersend request minimum interval seconds: value(%u).", m_uTimersendReqMinInterval);
}

void CHttpServiceThread::packetNormalJsonReply(TMsg *pMsg,
        const CHttpServiceSession *const pSession,
        const SessionMap::iterator iter,
        Json::Value &Jsondata
                                              )
{
    CHK_NULL_RETURN(pMsg);

    CUnityToHttpServerRespMsg *pUnity = (CUnityToHttpServerRespMsg *)pMsg;

    if (pUnity->m_vecCorrectPhone.empty() &&  pUnity->m_vecFarmatErrorPhone.empty()
            && pUnity->m_vecParamErrorPhone.empty() && pUnity->m_vecNomatchErrorPhone.empty())
    {
        LogError("this is error iSession[%lu]!", pUnity->m_uSessionId);

        //check if need change  YXZ errorCode to pure ErrCode
        int code = -6;
        string strMsg = m_mapHttpErrCodeDes[code];
        Json::Value tmpValue;

        tmpValue["code"] = Json::Value(code);
        tmpValue["msg"] = Json::Value(strMsg);
        tmpValue["fee"] = Json::Value(0);
        tmpValue["mobile"] = Json::Value("");
        tmpValue["sid"] = Json::Value(getUUID());

        if (HttpProtocolType_TX_Json == pSession->m_ucHttpProtocolType)
        {
            tmpValue["detail"] = Json::Value(strMsg);
        }
        else
        {
            tmpValue["uid"] = Json::Value(pSession->m_strUid);
        }

        Jsondata.append(tmpValue);
    }
    else
    {
        HandlePhoneList(pMsg, iter, pUnity->m_vecCorrectPhone, Jsondata, HTTPS_RESPONSE_CODE_0);
        HandlePhoneList(pMsg, iter, pUnity->m_vecFarmatErrorPhone, Jsondata, HTTPS_RESPONSE_CODE_7);
        HandlePhoneList(pMsg, iter, pUnity->m_vecParamErrorPhone, Jsondata, pUnity->m_iErrorCode);
        HandlePhoneList(pMsg, iter, pUnity->m_vecNomatchErrorPhone, Jsondata, HTTPS_RESPONSE_CODE_17);
        HandlePhoneList(pMsg, iter, pUnity->m_vecBlackListPhone, Jsondata, HTTPS_RESPONSE_CODE_29);
        HandlePhoneList(pMsg, iter, pUnity->m_vecRepeatPhone, Jsondata, HTTPS_RESPONSE_CODE_30);
    }

}
void CHttpServiceThread::packetTimerSendJsonReply(TMsg *pMsg,
        const CHttpServiceSession *const pSession,
        const SessionMap::iterator iter,
        Json::Value &Jsondata)
{
    CHK_NULL_RETURN(pMsg);

    CUnityToHttpServerRespMsg *pUnity = (CUnityToHttpServerRespMsg *)pMsg;

    int iErrCode = pUnity->m_iErrorCode;
    if (pUnity->m_vecCorrectPhone.empty() && pUnity->m_vecFarmatErrorPhone.empty()
            && pUnity->m_vecParamErrorPhone.empty() && pUnity->m_vecNomatchErrorPhone.empty())
    {
        iErrCode = HTTPS_RESPONSE_CODE_6;
    }
    // 和号码无关的请求，不用组装号码段
    if (iErrCode != HTTPS_RESPONSE_CODE_0
            && iErrCode != HTTPS_RESPONSE_CODE_7
            && iErrCode != HTTPS_RESPONSE_CODE_17
            && iErrCode != HTTPS_RESPONSE_CODE_29
            && iErrCode != HTTPS_RESPONSE_CODE_30)
    {
        map<int, string>::const_iterator iterErrMsg;
        CHK_MAP_FIND_INT_RET(m_mapHttpErrCodeDes, iterErrMsg, iErrCode);
        const string &strMessage = iterErrMsg->second;

        Json::Value tmpValue;
        tmpValue["code"] = Json::Value(iErrCode);
        tmpValue["msg"] = Json::Value(strMessage);
        Jsondata.append(tmpValue);
    }
    else
    {
        HandleTimerSendPhoneList(pMsg, iter, pUnity->m_vecCorrectPhone, Jsondata, HTTPS_RESPONSE_CODE_0);
        HandleTimerSendPhoneList(pMsg, iter, pUnity->m_vecFarmatErrorPhone, Jsondata, HTTPS_RESPONSE_CODE_7);
        HandleTimerSendPhoneList(pMsg, iter, pUnity->m_vecNomatchErrorPhone, Jsondata, HTTPS_RESPONSE_CODE_17);
        HandleTimerSendPhoneList(pMsg, iter, pUnity->m_vecBlackListPhone, Jsondata, HTTPS_RESPONSE_CODE_29);
        HandleTimerSendPhoneList(pMsg, iter, pUnity->m_vecRepeatPhone, Jsondata, HTTPS_RESPONSE_CODE_30);
    }
}

int CHttpServiceThread::transferToMLErr(int err)
{
    switch(err)
    {
    case HTTPS_RESPONSE_CODE_0:
    {
        return 0;
    }
    case HTTPS_RESPONSE_CODE_1:
    {
        return MLERR_MISSING_USERNAME;
    }
    case HTTPS_RESPONSE_CODE_2:
    {
        return MLERR_APIKEY_ORPASSWORD_ERR;
    }
    case HTTPS_RESPONSE_CODE_3:
    {
        return MLERR_ACCOUNTCANCLE;
    }
    case HTTPS_RESPONSE_CODE_4:
    {
        return MLERR_ACCOUNTBLOCK;
    }
    case HTTPS_RESPONSE_CODE_5:
    {
        return MLERR_UNANTHORIZED_IP;
    }
    case HTTPS_RESPONSE_CODE_6:
    {
        return MLERR_ACCOUNTFROZEN;
    }
    case HTTPS_RESPONSE_CODE_7:
    {
        return MLERR_PHONE_FORMATE_ERR;
    }
    case HTTPS_RESPONSE_CODE_8:
    {
        return MLERR_CONTENT_OVER_LONG;
    }
    case HTTPS_RESPONSE_CODE_9:
    {
        return MLERR_SIGN_UNKNOWN;
    }
    case HTTPS_RESPONSE_CODE_10:
    {
        return MLERR_PROTOCAL_ERR;
    }
    case HTTPS_RESPONSE_CODE_17:
    {
        return MLERR_PHONE_FOREIGN_NOT;
    }
    case HTTPS_RESPONSE_CODE_22:
    {
        return MLERR_ACCOUNTFROZEN;
    }
    case HTTPS_RESPONSE_CODE_24:
    {
        return MLERR_MISSING_MESSAGE_CONTENT;
    }
    case HTTPS_RESPONSE_CODE_25:
    {
        return MLERR_PHONE_TOO_MANY;
    }
    case HTTPS_RESPONSE_CODE_26:
    {
        return MLERR_MISSING_SIGN;
    }
    case HTTPS_RESPONSE_CODE_27:
    {
        return MLERR_SIGN_SHORT;
    }
    case HTTPS_RESPONSE_CODE_28:
    {
        return  MLERR_SIGN_LONG;
    }
    case HTTPS_RESPONSE_CODE_29:
    {
        return  MLERR_PHONE_BLACK;
    }
    case HTTPS_RESPONSE_CODE_30:
    {
        return  MLERR_PHONE_REPEAT;
    }
    case HTTPS_RESPONSE_CODE_31:
    {
        return MLERR_UNRECOGNIZED_ENCODING;
    }
    default:
        return MLERR;
    }
}

void CHttpServiceThread::HandleUnityRespMsg(TMsg *pMsg)
{
    CHK_NULL_RETURN(pMsg);

    CUnityToHttpServerRespMsg *pUnity = (CUnityToHttpServerRespMsg *)pMsg;

    SessionMap::iterator iter;
    CHK_MAP_FIND_UINT_RET(m_SessionMap, iter, pUnity->m_uSessionId);

    const CHttpServiceSession *const pSession = iter->second;
    CHK_NULL_RETURN(pSession);

    http::HttpResponse respone;
    string content = "";
    respone.SetStatusCode(200);
    Json::Value root;
    Json::Value Jsondata;
    char xmldata[512] = { 0 };

    if(SMS_FROM_ACCESS_HTTP == pSession->m_uSmsfrom &&
            (HttpProtocolType_JD_Webservice == pSession->m_ucHttpProtocolType
             || HttpProtocolType_JD == pSession->m_ucHttpProtocolType))
    {
    }
    else
    {
        if(HttpProtocolType_ML != pSession->m_ucHttpProtocolType)
        {
            if (!pUnity->m_bTimerSend)
            {
                packetNormalJsonReply(pMsg, pSession, iter, Jsondata);
            }
            else
            {
                packetTimerSendJsonReply(pMsg, pSession, iter, Jsondata);
            }
        }
    }

    if (SMS_FROM_ACCESS_HTTP == pSession->m_uSmsfrom)
    {
        if (HttpProtocolType_ML == pSession->m_ucHttpProtocolType)
        {
            memset(xmldata, 0, sizeof(xmldata));
            if (!pUnity->m_vecCorrectPhone.empty() && !pUnity->m_vecFarmatErrorPhone.empty()
                    && !pUnity->m_vecParamErrorPhone.empty() && !pUnity->m_vecNomatchErrorPhone.empty())
            {
                snprintf(xmldata, sizeof(xmldata), "%s", ML_error_reply[MLERR_MISSING_RECIPIENT].data());
            }
            else if (!pUnity->m_vecCorrectPhone.empty())
            {
                if (!pUnity->m_vecParamErrorPhone.empty() || !pUnity->m_vecFarmatErrorPhone.empty()
                        || !pUnity->m_vecNomatchErrorPhone.empty() || !pUnity->m_vecBlackListPhone.empty()
                        || !pUnity->m_vecRepeatPhone.empty())
                {
                    snprintf(xmldata, sizeof(xmldata), "error:%s", pUnity->m_strSmsid.data());
                    LogDebug("***********error******** submitid: %s", pUnity->m_strSmsid.data());
                }
                else
                {
                    snprintf(xmldata, sizeof(xmldata), "success:%s", pUnity->m_strSmsid.data());
                    LogDebug("***********success****** submitid: %s", pUnity->m_strSmsid.data());
                }
            }
            else
            {
                if(!pUnity->m_vecParamErrorPhone.empty())
                {
                    snprintf(xmldata, sizeof(xmldata), "%s", ML_error_reply[transferToMLErr(pUnity->m_iErrorCode)].data());
                }
                else if(!pUnity->m_vecFarmatErrorPhone.empty())
                {
                    snprintf(xmldata, sizeof(xmldata), "%s", ML_error_reply[transferToMLErr(HTTPS_RESPONSE_CODE_7)].data());
                }
                else if(!pUnity->m_vecNomatchErrorPhone.empty())
                {
                    snprintf(xmldata, sizeof(xmldata), "%s", ML_error_reply[transferToMLErr(HTTPS_RESPONSE_CODE_17)].data());
                }
                else if(!pUnity->m_vecBlackListPhone.empty())
                {
                    snprintf(xmldata, sizeof(xmldata), "%s", ML_error_reply[transferToMLErr(HTTPS_RESPONSE_CODE_29)].data());
                }
                else if (!pUnity->m_vecRepeatPhone.empty())
                {
                    snprintf(xmldata, sizeof(xmldata), "%s", ML_error_reply[transferToMLErr(HTTPS_RESPONSE_CODE_30)].data());
                }
                else
                {
                    LogError("*******************error phone");
                }
            }
        }
        else if (HttpProtocolType_TX_Json == pSession->m_ucHttpProtocolType)
        {
            string str1 = "%e6%8f%90%e4%ba%a4%e6%88%90%e5%8a%9f%ef%bc%81";  // submit success
            str1 = http::UrlCode::UrlDecode(str1);
            root["code"] = Json::Value("0");
            root["msg"] = Json::Value(str1);
            root["result"] = Jsondata;
        }
        else if(HttpProtocolType_JD_Webservice == pSession->m_ucHttpProtocolType
                || HttpProtocolType_JD == pSession->m_ucHttpProtocolType)
        {
            memset(xmldata, 0, sizeof(xmldata));
            if(!pUnity->m_vecFarmatErrorPhone.empty())
            {
                snprintf(xmldata, sizeof(xmldata) - 1, JD_SENDSMS_RESP_MSG, RESP_CODE_JD_INVALID_MOBILE);
            }
            else if(!pUnity->m_vecNomatchErrorPhone.empty())
            {
                snprintf(xmldata, sizeof(xmldata) - 1, JD_SENDSMS_RESP_MSG, RESP_CODE_JD_GENERAL_ERR);
            }
            else if(!pUnity->m_vecBlackListPhone.empty())
            {
                snprintf(xmldata, sizeof(xmldata) - 1, JD_SENDSMS_RESP_MSG, RESP_CODE_JD_GENERAL_ERR);
            }
            else if(!pUnity->m_vecRepeatPhone.empty())
            {
                snprintf(xmldata, sizeof(xmldata) - 1, JD_SENDSMS_RESP_MSG, RESP_CODE_JD_INVALID_MOBILELIST);
            }
            else
            {
                snprintf(xmldata, sizeof(xmldata) - 1,
                         JD_SENDSMS_RESP_MSG, pUnity->m_strSmsid.c_str());

                printf("******************* smsid: %s\n", pUnity->m_strSmsid.c_str());
            }
        }
        else
        {
            int totalsuc = pUnity->m_vecCorrectPhone.size() * pSession->m_uFeeNum;
            root["total_fee"] = Json::Value(totalsuc);
            root["data"] = Jsondata;
            // 定时短信的返回的uid挪到根部
            if (pUnity->m_bTimerSend)
            {
                root["uid"] = Json::Value(pSession->m_strUid);
                root["sid"] = Json::Value(pUnity->m_strSmsid);
            }
        }
    }

    Json::FastWriter fast_writer;
    if (SMS_FROM_ACCESS_HTTP == pSession->m_uSmsfrom)
    {
        if (HttpProtocolType_ML == pSession->m_ucHttpProtocolType)
        {
            respone.SetContent(xmldata);
            respone.EncodeTextHtml(content);
        }
        else if (HttpProtocolType_JD_Webservice == pSession->m_ucHttpProtocolType
                 || HttpProtocolType_JD == pSession->m_ucHttpProtocolType)
        {
            respone.SetContent(xmldata);
            respone.Encode(content);
        }
        else
        {
            respone.SetContent(fast_writer.write(root));
            respone.Encode(content);
        }
    }
    else
    {
        respone.SetContent(fast_writer.write(Jsondata));
        respone.Encode(content);
    }

    if(pSession->m_webHandler && pSession->m_webHandler->m_socket)
    {
        LogNotice("smsp_http push httpResp to client message:%s. pSession: %d, HttpProtocolType: %d",
                  content.data(), pSession->m_uSmsfrom, pSession->m_ucHttpProtocolType);
        pSession->m_webHandler->m_socket->Out()->Write(content.data(), content.size());
        pSession->m_webHandler->m_socket->Out()->Flush();
    }
    else
    {
        LogWarn("webhandle->m_socket or webhandler is NULL");
    }

    LogElk("smsp_http push httpResp to client message end.");
    return;
}

void CHttpServiceThread::HandlePhoneList(TMsg *pMsg,
        const SessionMap::iterator iter,
        const vector<string> &vecPhoneList,
        Json::Value &Jsondata,
        int iErrCode)
{
    CHK_NULL_RETURN(pMsg);

    CUnityToHttpServerRespMsg *pUnity = (CUnityToHttpServerRespMsg *)pMsg;

    map<int, string>::const_iterator iterErrMsg;
    CHK_MAP_FIND_INT_RET(m_mapHttpErrCodeDes, iterErrMsg, iErrCode)

    const string &strMessage = iterErrMsg->second;

    for(vector<string>::const_iterator itrFormat = vecPhoneList.begin();
            itrFormat != vecPhoneList.end(); ++itrFormat)
    {
        Json::Value tmpValue;
        tmpValue["msg"] = Json::Value(strMessage);
        tmpValue["mobile"] = Json::Value(*itrFormat);

        if (HTTPS_RESPONSE_CODE_0 == iErrCode)
        {
            tmpValue["fee"] = Json::Value(iter->second->m_uFeeNum);
        }
        else
        {
            tmpValue["fee"] = Json::Value(0);
        }

        if (HttpProtocolType_TX_Json == iter->second->m_ucHttpProtocolType)
        {
            string strSid = "";
            map<string, string>::iterator iter;
            iter = pUnity->m_mapPhoneSmsId.find((*itrFormat).data());

            if (pUnity->m_mapPhoneSmsId.end() != iter)
            {
                strSid = iter->second;
            }

            tmpValue["code"] = Json::Value(Comm::int2str(iErrCode)); // 0 and "0"
            tmpValue["sid"] = Json::Value(strSid);

            if (HTTPS_RESPONSE_CODE_0 == iErrCode)
            {
                tmpValue["dsisplay_number"] = Json::Value(pUnity->m_strDisplayNum);
            }
            else if (HTTPS_RESPONSE_CODE_25 == iErrCode)
            {
                // modify the "msg" and "detail" field
                // The number of send phone exceed 200
                static string str = http::UrlCode::UrlDecode("%e5%8f%91%e9%80%81%e5%8f%b7%e7%a0%81%e6%95%b0%e9%87%8f%e8%b6%85%e8%bf%87200%e4%b8%aa");
                tmpValue["msg"] = Json::Value(str);
                tmpValue["detail"] = Json::Value(str);
            }
            else
            {
                tmpValue["detail"] = Json::Value(strMessage);
            }
        }
        else
        {
            tmpValue["code"] = Json::Value(iErrCode);
            tmpValue["sid"] = Json::Value(pUnity->m_strSmsid);
            tmpValue["uid"] = Json::Value(iter->second->m_strUid);
        }

        Jsondata.append(tmpValue);
    }
}
void CHttpServiceThread::HandleTimerSendPhoneList(TMsg *pMsg,
        const SessionMap::iterator iter,
        const vector<string> &vecPhoneList,
        Json::Value &Jsondata,
        int iErrCode)
{
    // 空的错误集合不需要回复
    if (vecPhoneList.empty())
    {
        return;
    }

    map<int, string>::const_iterator iterErrMsg;
    CHK_MAP_FIND_INT_RET(m_mapHttpErrCodeDes, iterErrMsg, iErrCode)

    const string &strMessage = iterErrMsg->second;

    Json::Value tmpValue;

    // 压缩号码列表，失败时组装，成功时不用组装
    if (HTTPS_RESPONSE_CODE_0 != iErrCode)
    {
        std::string strPhoneList = "";

        for(vector<string>::const_iterator itrFormat = vecPhoneList.begin();
                itrFormat != vecPhoneList.end(); ++itrFormat)
        {
            strPhoneList.append(*itrFormat);
            strPhoneList.append(",");
        }
        // remove last ','
        strPhoneList.resize(strPhoneList.size() - 1);
        LogDebug("Reply error phonelist: [%d]", strPhoneList.size());

        std::string strCompressedPhoneList;

        //TODO:传送iCompressType
        int ret = CompressHelper::compress(GZIP, strPhoneList, strCompressedPhoneList);
        if (ret != 0)
        {
            LogError("compress mobilelist failed. ret:[%d]", ret);
            return;
        }

        strPhoneList = Base64::Encode(strCompressedPhoneList);
        tmpValue["mobilelist"] = Json::Value(strPhoneList);
    }
    tmpValue["msg"] = Json::Value(strMessage);
    tmpValue["mobilecnt"] = Json::Value((int)vecPhoneList.size());
    tmpValue["code"] = Json::Value(iErrCode);

    Jsondata.append(tmpValue);
}

void CHttpServiceThread::HandleHTTPServiceAcceptSocketMsg(TMsg *pMsg)
{
    TAcceptSocketMsg *pTAcceptSocketMsg = (TAcceptSocketMsg *)pMsg;
    CHK_NULL_RETURN(pTAcceptSocketMsg);
    CHK_NULL_RETURN(m_pSnManager);

    UInt64 ullSeq = m_pSnManager->getSn();

    AccessHttpSocketHandler *pSendSocketHandler = new AccessHttpSocketHandler(this, ullSeq);
    CHK_NULL_RETURN(pSendSocketHandler);
    if(false == pSendSocketHandler->Init(m_pInternalService, pTAcceptSocketMsg->m_iSocket, pTAcceptSocketMsg->m_address))
    {
        LogError("CHttpServiceThread::HandleHTTPServiceAcceptSocketMsg is faield.")
		SAFE_DELETE(pSendSocketHandler);
        return;
    }

    CHttpServiceSession *pSession = new CHttpServiceSession();	//CHttpServiceSession SMSServiceSession
    CHK_NULL_RETURN(pSession);
    pSession->m_webHandler = pSendSocketHandler;
    pSession->m_wheelTimer = SetTimer(ullSeq, "ACCESS_HTTPSERVER_SESSION_TIMEOUT", 6 * 1000);	//SMS_session 6s timeout
    m_SessionMap[ullSeq] = pSession;
}

bool CHttpServiceThread::validAndPreHandleTimerSendReq(const std::string &strAccount,
        const Json::Value &root,
        UInt32 &uTimerSendSubmittype,
        std::string &strTimerSendTime,
        std::string &strPhone,
        int &iRespCode)
{
    std::string strTimerSendSubmittype = root["submittype"].asString();
    if (!strTimerSendSubmittype.empty())
    {
        if (strTimerSendSubmittype != "0" /* subclient */
            && strTimerSendSubmittype != "1" /* agent */)
        {
            LogError("account:%s, Invalid 'submittype':[%s], must be '0/1'", strAccount.c_str(), strTimerSendSubmittype.c_str());
            iRespCode = HTTPS_RESPONSE_CODE_1; // TODO:add new errorcode
            return false;
        }
        uTimerSendSubmittype = atoi(strTimerSendSubmittype.c_str());
    }

    // CheckPoint 1: 客户为http协议用户
    AccountMap::iterator itAccnt = m_mapAccount.find(strAccount);
    if (itAccnt == m_mapAccount.end())
    {
        LogError("account[%s] doesn't exist!", strAccount.c_str());
        iRespCode = HTTPS_RESPONSE_CODE_1;
        return false;
    }

    if (itAccnt->second.m_uSmsFrom != BusType::SMS_FROM_ACCESS_HTTP
            || itAccnt->second.m_uAgentId < 2000000000 )
    {
        LogError("account[%s] not http-protocol-client[protocol:%d], or not belong to an agent[belong:%u]!",
                 strAccount.c_str(), itAccnt->second.m_uSmsFrom, itAccnt->second.m_uAgentId);
        iRespCode = HTTPS_RESPONSE_CODE_35;

        return false;
    }

    // CheckPoint 2: 客户的代理商类型为OEM
    map< UInt64, AgentInfo >::iterator itAgent = m_AgentInfoMap.find( itAccnt->second.m_uAgentId );
    if( itAgent == m_AgentInfoMap.end())
    {
        LogError("client[%s]'s agent[%u] not found in m_AgentInfoMap.", strAccount.c_str(),
                 itAccnt->second.m_uAgentId);
        iRespCode = HTTPS_RESPONSE_CODE_14;

        return false;
    }

    if (itAgent->second.m_iAgent_type != BusType::AGENTTYPE_OEM)
    {
        LogError("account[%s]'s agent[%u] not OEM[type:%d]!",
                 strAccount.c_str(), itAccnt->second.m_uAgentId,
                 itAgent->second.m_iAgent_type
                );

        iRespCode = HTTPS_RESPONSE_CODE_35;

        return false;
    }

    // CheckPoint 3: 检查定时短信时间格式
    strTimerSendTime = root["sendtime"].asString();
    if (!Comm::checkTimeFormat(strTimerSendTime))
    {
        LogError("account[%s]: invalid timersend's time-format[%s]!", strAccount.c_str(), strTimerSendTime.c_str());

        iRespCode = HTTPS_RESPONSE_CODE_33;
        return false;
    }

    // CheckPoint 4: 检查时间间隔
    time_t now_time = time(NULL);
    time_t send_time = Comm::asctime2seconds(strTimerSendTime.c_str(), "%Y-%m-%d %H:%M:%S");
    // time elapsed from send_time ~~~~> now_time
    double interval = difftime(send_time, now_time);
    if (interval < m_uTimersendReqMinInterval * 60)
    {
        LogError("account[%s]'s sms too short sendtime-interval: %lf [ %ld - %ld] mininterval:[%us]",
                 strAccount.c_str(),
                 interval,
                 now_time,
                 send_time,
                 m_uTimersendReqMinInterval * 60);
        iRespCode = HTTPS_RESPONSE_CODE_34;
        return false;
    }

    // 解压缩
    COMPRESS_TYPE iCompressType = (COMPRESS_TYPE)atoi(root["compress_type"].asString().c_str());
    strPhone = root["mobilelist"].asString();
    LogDebug("mobilist: %s", strPhone.c_str());

    // Step1: Base64-Decode
    std::string strPhoneTmp = Base64::Decode(strPhone);
    strPhone = "";

    // Step2: gzipCompress
    int ret = CompressHelper::decompress(iCompressType, strPhoneTmp, strPhone);
    if (ret != 0)
    {
        iRespCode = HTTPS_RESPONSE_CODE_36;
        LogError("account:%s Decompress mobilelist failed. ret:[%d]", strAccount.c_str(), ret);
        return false;
    }
    else
    {
        LogDebug("account:%s Decompressed mobilelist: %s.", strAccount.c_str(), strPhone.c_str());
    }

    return true;
}

void CHttpServiceThread::HandleHTTPServerReqMsg(TMsg *pMsg)
{
    THTTPRequest *pHTTPRequest = (THTTPRequest *)pMsg;
    CHK_NULL_RETURN(pHTTPRequest);

    SessionMap::iterator itSession = m_SessionMap.find(pHTTPRequest->m_iSeq);
    if(itSession == m_SessionMap.end())
    {
        LogWarn("iSeq[%lu] is not find m_SessionMap, receive one http message:%s", pHTTPRequest->m_iSeq, pHTTPRequest->m_strRequest.c_str());
        return;
    }

    itSession->second->m_strGustIP = pHTTPRequest->m_socketHandle->GetServerIpAddress();
    itSession->second->m_ucHttpProtocolType = pHTTPRequest->m_ucHttpProtocolType;
    itSession->second->m_bTimerSend = (MSGTYPE_TIMERSEND_SMSERVICE_REQ == pMsg->m_iMsgType) ? true : false;

    LogNotice("smsp_http receive one http message, msgtype(%x), iSeq(%ld), Guest ip(%s).",
              pMsg->m_iMsgType, pHTTPRequest->m_iSeq, itSession->second->m_strGustIP.c_str());

    string strPhone = "";
    string strContent = "";
    string strExtendPort = "";
    string strUid = "";
    string strAccount = "";
    string strPwdMd5 = "";
    string strSmsType = "";
    string strChannelId = "";
    string strTimerSendTime = "";
    UInt32 uTimerSendSubmittype = TimerSendSubmitType_SubClient;

    try
    {
        Json::Reader reader(Json::Features::strictMode());
        Json::Value root;
        std::string js = "";

        if (json_format_string(pHTTPRequest->m_strRequest, js) < 0)
        {
            LogError("json message error, req[%s]", pHTTPRequest->m_strRequest.data());
            // failed reponse
            proHttpsExecpt(HTTPS_RESPONSE_CODE_20, pHTTPRequest->m_iSeq);
            return;
        }
        if(!reader.parse(js, root))
        {
            LogError("json parse is failed, req[%s]", pHTTPRequest->m_strRequest.data());
            // failed reponse
            proHttpsExecpt(HTTPS_RESPONSE_CODE_21, pHTTPRequest->m_iSeq);
            return;
        }

        if (HttpProtocolType_TX_Json == pHTTPRequest->m_ucHttpProtocolType)
        {
            strAccount = root["account"].asString();
            strPwdMd5 = root["pwd"].asString();
        }
        else
        {
            strAccount = root["clientid"].asString();
            strPwdMd5 = root["password"].asString();
            strSmsType = root["smstype"].asString();
            strChannelId = root["channelid"].asString();
        }

        strContent = root["content"].asString();
        strExtendPort = root["extend"].asString();
        strUid = root["uid"].asString();

        trim(strUid);
        itSession->second->m_strUid = strUid;

        if (strUid.length() > 60)
        {
            LogError("account:%s uid[%s] over length !", strAccount.c_str(), strUid.c_str());
            proHttpsExecpt(HTTPS_RESPONSE_CODE_37, pHTTPRequest->m_iSeq);
            return;
        }

        if (itSession->second->m_bTimerSend)
        {
            int iRespCode = HTTPS_RESPONSE_CODE_0;

            if (validAndPreHandleTimerSendReq(strAccount, root, uTimerSendSubmittype, strTimerSendTime, strPhone, iRespCode) == false)
            {
                proHttpsExecpt(iRespCode, pHTTPRequest->m_iSeq);
                return;
            }
        }
        else
        {
            strPhone = root["mobile"].asString();
        }

        trim(strAccount);
        trim(strPwdMd5);
        trim(strExtendPort);
        trim(strUid);
        trim(strPhone);
        trim(strSmsType);
        trim(strChannelId);

        if(!Comm::isNumber(strExtendPort))
        {
            LogError("access:%s httpserver ExtendPort error. Request[%s]", strAccount.c_str(), strExtendPort.c_str());
            proHttpsExecpt(HTTPS_RESPONSE_CODE_38, pHTTPRequest->m_iSeq);
            strExtendPort = "";
            return;
        }
    }
    catch(...)
    {
        LogError("try catch access httpserver unpacet error, json err. Request[%s]", pHTTPRequest->m_strRequest.data());
        // failed reponse
        proHttpsExecpt(HTTPS_RESPONSE_CODE_21, pHTTPRequest->m_iSeq);
        return;
    }

    if (strPhone.empty())
    {
        LogError("==err== strPhone[%s] is empty!", strPhone.data());
        // phone is empty must respone
        proHttpsExecpt(HTTPS_RESPONSE_CODE_6, pHTTPRequest->m_iSeq);
        return;
    }

    if ((HttpProtocolType_TX_Json != pHTTPRequest->m_ucHttpProtocolType)
            && (strSmsType.empty()))
    {
        LogError("account:%s strSmsType:%s is empty!", strAccount.c_str(), strSmsType.c_str());
        // strSmsType is empty must respone
        proHttpsExecpt(HTTPS_RESPONSE_CODE_23, pHTTPRequest->m_iSeq);
        return;
    }

    // 先校验账号是否是6位
    if (6 != strAccount.length())
    {
        LogError("==err== account[%s] does't exist!", strAccount.c_str());
        proHttpsExecpt(HTTPS_RESPONSE_CODE_1, pHTTPRequest->m_iSeq);
        return;
    }

    if (strSmsType.empty())
    {
        LogError("==err== account[%s],strSmsType[%s] is empty!", strAccount.c_str(), strSmsType.data());
        //// strSmsType is empty must respone
        proHttpsExecpt(HTTPS_RESPONSE_CODE_23, pHTTPRequest->m_iSeq, false);
        return;
    }

    // 判断短信类型
    int nSmsType = atoi(strSmsType.c_str());
    if (0 != nSmsType && 4 != nSmsType && 5 != nSmsType && 6 != nSmsType
            && 7 != nSmsType && 8 != nSmsType && 9 != nSmsType)
    {
        LogError("==err== account[%s],strPhone[%s] smstype [%s] error!", strAccount.c_str(), strPhone.c_str(), strSmsType.c_str());
        proHttpsExecpt(HTTPS_RESPONSE_CODE_23, pHTTPRequest->m_iSeq);
        return;
    }

    utils::UTFString utfHelper;
    UInt32 msgLength = utfHelper.getUtf8Length(strContent);
    UInt32 uRate = (msgLength <= 70 ? 1 : ((msgLength + 66) / 67));

    itSession->second->m_uFeeNum = uRate;
    itSession->second->m_uSmsfrom = SMS_FROM_ACCESS_HTTP;

    CHttpServerToUnityReqMsg *pHttps = new CHttpServerToUnityReqMsg();
    CHK_NULL_RETURN(pHttps);
    
    pHttps->m_strAccount.assign(strAccount);
    pHttps->m_strContent.assign(strContent);
    pHttps->m_strExtendPort.assign(strExtendPort);
    pHttps->m_strPwdMd5.assign(strPwdMd5);
    pHttps->m_strUid.assign(strUid);
    pHttps->m_uSmsFrom = SMS_FROM_ACCESS_HTTP;
    pHttps->m_strPhone.assign(strPhone);
    pHttps->m_uSessionId = pHTTPRequest->m_iSeq;
    pHttps->m_strIp = itSession->second->m_strGustIP;
    pHttps->m_strSmsType = strSmsType.data();
    pHttps->m_uLongSms = (uRate == 1 ? 0 : 1);
    pHttps->m_strChannelId.assign(strChannelId);
    pHttps->m_iMsgType = MSGTYPE_HTTPS_TO_UNITY_REQ;
    pHttps->m_ucHttpProtocolType = pHTTPRequest->m_ucHttpProtocolType;
    pHttps->m_bTimerSend = itSession->second->m_bTimerSend;
    pHttps->m_strTimerSendTime = strTimerSendTime;
    pHttps->m_uTimerSendSubmittype = uTimerSendSubmittype;
    g_pUnitypThread->PostMsg(pHttps);

    return;
}

void CHttpServiceThread::proHttpsExecpt_ML(int uErrorCode, UInt64 uSessionId, bool bNormal)
{
    SessionMap::iterator iter;
    CHK_MAP_FIND_UINT_RET(m_SessionMap, iter, uSessionId);

    const CHttpServiceSession *const pSession = iter->second;
    CHK_NULL_RETURN(pSession);

    http::HttpResponse respone;
    respone.SetStatusCode(200);
    string strContent = "", strData = "";

    if (bNormal)
    {
        strData.assign("success:");
        strData.append(getUUID());
    }
    else
    {
        if (MLERR == uErrorCode)
        {
            strData.assign(ML_error_reply[uErrorCode]);
            strData.append(getUUID());
        }
        else
        {
            if (uErrorCode >= ML_MAX_ERR)
            {
                LogError("errorCode[%d] larger than %d", uErrorCode, ML_MAX_ERR);
                strData.assign(ML_error_reply[MLERR]);
            }
            else
            {
                strData.assign(ML_error_reply[uErrorCode]);
            }
        }
    }

    respone.SetContent(strData);
    respone.EncodeTextHtml(strContent);

    if(pSession->m_webHandler && pSession->m_webHandler->m_socket)
    {
        pSession->m_webHandler->m_socket->Out()->Write(strContent.data(), strContent.size());
        pSession->m_webHandler->m_socket->Out()->Flush();
    }
    else
    {
        LogWarn("webhandle->m_socket or webhandler is NULL");
    }
    return;
}

void CHttpServiceThread::HandleHTTPServerReqMsg_ML(TMsg *pMsg)
{
    THTTPRequest *pHTTPRequest = (THTTPRequest *)pMsg;
    CHK_NULL_RETURN(pHTTPRequest);

    SessionMap::iterator itSession = m_SessionMap.find(pHTTPRequest->m_iSeq);
    if(itSession == m_SessionMap.end())
    {
        LogWarn("iSeq[%lu] is not find m_SessionMap, receive one http message:%s", pHTTPRequest->m_iSeq, pHTTPRequest->m_strRequest.c_str());
        return;
    }

    itSession->second->m_strGustIP = pHTTPRequest->m_socketHandle->GetServerIpAddress();
    itSession->second->m_ucHttpProtocolType = pHTTPRequest->m_ucHttpProtocolType;

    LogNotice("smsp_http receive one http message, msgtype(%x), iSeq(%ld), Guest ip(%s).",
              pMsg->m_iMsgType, pHTTPRequest->m_iSeq, itSession->second->m_strGustIP.c_str());

    std::map<std::string, std::string> mapSet;
    Comm::splitExMap(pHTTPRequest->m_strRequest, "&", mapSet);

    string strPhone = "";;
    string strContent = "";
    string strConOrigin = "";
    string strExtendPort = "";
    string strUid = "";
    string strAccount = "";
    string strPwdMd5 = "";
    string strPwd = "";
    string strSmsType = "";
    string strChannelId = "";
    string strEncode = "";

    strAccount = mapSet["username"];
    strPwd = mapSet["password"];//password_md5
    
    Comm::trim(strPwd);
    
    if (!strPwd.empty())
    {
        strPwdMd5 = Comm::GetStrMd5(strPwd);
    }

    strPhone = mapSet["mobile"];
    strContent = strConOrigin = http::UrlCode::UrlDecode(mapSet["content"]);
    strExtendPort = mapSet["ext"];
    strEncode = mapSet["encode"];
    Comm::trim(strAccount);

    Comm::trim(strExtendPort);
    Comm::trim(strPhone);

    if (!Comm::isAscii(strConOrigin))
    {
        utils::UTFString utfHelper;
        strContent.clear();
        utfHelper.g2u(strConOrigin, strContent);
        LogDebug("transfer gbk to utf-8, %s", strContent.data());
    }

    if(!Comm::isNumber(strExtendPort))
    {
        LogError("access:%s httpserver ExtendPort error. Request[%s]", strAccount.c_str(), strExtendPort.c_str());
        proHttpsExecpt(HTTPS_RESPONSE_CODE_38, pHTTPRequest->m_iSeq);
        return;
    }

    AccountMap::iterator itAccnt = m_mapAccount.find(strAccount);
    if (itAccnt == m_mapAccount.end())
    {
        LogError("account:%s doesn't exist!", strAccount.c_str());
        proHttpsExecpt_ML(MLERR_MISSING_USERNAME, pHTTPRequest->m_iSeq, false);
        return;
    }
    strSmsType = itAccnt->second.m_strSmsType;

    if (strSmsType.empty())
    {
        LogError("account:%s,strSmsType:%s is empty!", strAccount.c_str(), strSmsType.data());
        // strSmsType is empty must respone
        proHttpsExecpt_ML(MLERR_SMSTYPE_EMPTY, pHTTPRequest->m_iSeq, false);
        return;
    }

    // 判断短信类型
    int nSmsType = atoi(strSmsType.c_str());
    if (0 != nSmsType && 4 != nSmsType && 5 != nSmsType && 6 != nSmsType
            && 7 != nSmsType && 8 != nSmsType && 9 != nSmsType)
    {
        LogError("==err== account[%s],strPhone[%s] smstype [%s] error!", strAccount.c_str(), strPhone.c_str(), strSmsType.c_str());
        proHttpsExecpt(HTTPS_RESPONSE_CODE_23, pHTTPRequest->m_iSeq);
        return;
    }

    if (strPhone.empty())
    {
        LogError("account:%s,strPhone:%s is empty!", strAccount.c_str(), strPhone.c_str());
        // phone is empty must respone
        proHttpsExecpt_ML(MLERR_MISSING_RECIPIENT, pHTTPRequest->m_iSeq, false);
        return;
    }


    utils::UTFString utfHelper;
    UInt32 msgLength = utfHelper.getUtf8Length(strContent);
    UInt32 uRate = (msgLength <= 70 ? 1 : ((msgLength + 66) / 67));

    itSession->second->m_uFeeNum = uRate;
    itSession->second->m_uSmsfrom = SMS_FROM_ACCESS_HTTP;

    CHttpServerToUnityReqMsg *pHttps = new CHttpServerToUnityReqMsg();
    CHK_NULL_RETURN(pHttps);
    pHttps->m_strAccount.assign(strAccount);
    pHttps->m_strContent.assign(strContent);
    pHttps->m_strExtendPort.assign(strExtendPort);
    pHttps->m_strPwdMd5.assign(strPwdMd5);
    pHttps->m_strUid.assign(strUid);
    pHttps->m_uSmsFrom = SMS_FROM_ACCESS_HTTP;
    pHttps->m_strPhone.assign(strPhone);
    pHttps->m_uSessionId = pHTTPRequest->m_iSeq;
    pHttps->m_strIp = itSession->second->m_strGustIP;
    pHttps->m_strSmsType = strSmsType;
    pHttps->m_uLongSms = (uRate == 1 ? 0 : 1);
    pHttps->m_strChannelId.assign(strChannelId);
    pHttps->m_iMsgType = MSGTYPE_HTTPS_TO_UNITY_REQ;
    pHttps->m_ucHttpProtocolType = pHTTPRequest->m_ucHttpProtocolType;
    g_pUnitypThread->PostMsg(pHttps);

    return;
}

void CHttpServiceThread::proHttpsExecpt(int uErrorCode, UInt64 uSessionId, bool bNormal)
{
    SessionMap::iterator iter;
    CHK_MAP_FIND_UINT_RET(m_SessionMap, iter, uSessionId);

    const CHttpServiceSession *const pSession = iter->second;
    CHK_NULL_RETURN(pSession);

    //check if need change  YXZ errorCode to pure ErrCode
    int nCode = uErrorCode;
    string strMsg = m_mapHttpErrCodeDes[nCode];


    http::HttpResponse respone;
    string strContent = "";
    Json::Value root;
    Json::Value Jsondata;
    Json::Value tmpValue;

    respone.SetStatusCode(200);

    if (!pSession->m_bTimerSend)
    {
        tmpValue["code"] = Json::Value(nCode);
        tmpValue["msg"] = Json::Value(strMsg);
        tmpValue["mobile"] = Json::Value("");
        tmpValue["sid"] = Json::Value(getUUID());

        if (HttpProtocolType_TX_Json == pSession->m_ucHttpProtocolType)
        {
            // need integer not string for tencent cloud
            tmpValue["fee"] = Json::Value(0);
            tmpValue["detail"] = Json::Value(strMsg);
        }
        else
        {
            tmpValue["fee"] = Json::Value("0");
            tmpValue["uid"] = Json::Value(pSession->m_strUid);
        }

        Jsondata.append(tmpValue);

        if (true == bNormal)
        {
            if (HttpProtocolType_TX_Json == pSession->m_ucHttpProtocolType)
            {
                // submit success
                string str1 = http::UrlCode::UrlDecode("%e6%8f%90%e4%ba%a4%e6%88%90%e5%8a%9f%ef%bc%81");
                root["code"] = Json::Value("0");
                root["msg"] = Json::Value(str1);
                root["result"] = Jsondata;
            }
            else
            {
                root["total_fee"] = Json::Value("0");
                root["data"] = Jsondata;
            }
        }
    }
    else
    {
        // 定时短信回复

        tmpValue["code"] = Json::Value(nCode);
        tmpValue["msg"] = Json::Value(strMsg);
        Jsondata.append(tmpValue);

        root["total_fee"] = Json::Value(0);
        root["uid"] = Json::Value(pSession->m_strUid);
        root["data"] = Jsondata;
        root["sid"] = Json::Value(getUUID());
    }

    Json::FastWriter fast_writer;
    if (bNormal)
    {
        respone.SetContent(fast_writer.write(root));
    }
    else
    {
        respone.SetContent(fast_writer.write(Jsondata));
    }

    respone.Encode(strContent);

    if(pSession->m_webHandler && pSession->m_webHandler->m_socket)
    {
        pSession->m_webHandler->m_socket->Out()->Write(strContent.data(), strContent.size());
        pSession->m_webHandler->m_socket->Out()->Flush();
    }
    else
    {
        LogWarn("webhandle->m_socket or webhandler is NULL");
    }

    return;

}

void CHttpServiceThread::proHttpsExecpt_JD(const char *jd_resp_code,
        int uErrorCode, UInt64 uSessionId)
{
    SessionMap::iterator iter = m_SessionMap.find(uSessionId);
    if (iter == m_SessionMap.end())
    {
        LogError("==err== uSessionId[%ld] is not find SessionMap!", uSessionId);
        return;
    }

    http::HttpResponse respone;
    respone.SetStatusCode(200);
    string content = "";
    string xmldata = "";
    char resp_msg[512] = { 0 };

    memset(resp_msg, 0, sizeof(resp_msg));
    if(NULL != jd_resp_code)
    {
        snprintf(resp_msg, sizeof(resp_msg) - 1,
                 JD_SENDSMS_RESP_MSG, jd_resp_code);
    }
    else
    {
        snprintf(resp_msg, sizeof(resp_msg) - 1,
                 JD_SENDSMS_RESP_MSG, Transfer2JDErrCode(uErrorCode));
    }

    //xmldata = resp_msg;
    respone.SetContent(resp_msg);
    respone.Encode(content);

    if(NULL != iter->second->m_webHandler && NULL != iter->second->m_webHandler->m_socket)
    {
        iter->second->m_webHandler->m_socket->Out()->Write(content.data(), content.size());
        iter->second->m_webHandler->m_socket->Out()->Flush();
    }
    else
    {
        LogWarn("webhandle->m_socket or webhandler is NULL");
    }

    return;
}

void CHttpServiceThread::HandleHTTPServiceReturnOverMsg(TMsg *pMsg)
{
    //write done
    SessionMap::iterator it = m_SessionMap.find(pMsg->m_iSeq);
    if(it != m_SessionMap.end())
    {
        SAFE_DELETE(it->second->m_wheelTimer);
        
        if(it->second->m_webHandler)
        {
            it->second->m_webHandler->Destroy();
            SAFE_DELETE(it->second->m_webHandler);
        }
        
        SAFE_DELETE(it->second);
        m_SessionMap.erase(it);
    }
}

void CHttpServiceThread::HandleHTTPResponse(UInt64 ullSeq, string &response)
{
    //给应答
    SessionMap::iterator it = m_SessionMap.find(ullSeq);
    if(it == m_SessionMap.end())
    {
        LogWarn("seq:%lu get session failed, parse over, return to user", ullSeq);
        return;
    }

    if(it->second->m_webHandler && it->second->m_webHandler->m_socket)
    {
        std::string strContent = "";
        http::HttpResponse respone;
        respone.SetStatusCode(200);
        respone.SetContent(response);
        respone.Encode(strContent);

        LogNotice("smsp_http query resp to client message:[%s]", strContent.c_str());
        it->second->m_webHandler->m_socket->Out()->Write(strContent.c_str(), strContent.size());
        it->second->m_webHandler->m_socket->Out()->Flush();
    }
    else
    {
        LogWarn("webhandle->m_socket or webhandler is NULL");
    }

    LogElk("smsp_http query resp to client end.");
}

UInt32 CHttpServiceThread::GetSessionMapSize()
{
    return m_SessionMap.size();
}

void CHttpServiceThread::HandleHTTPServerReqMsg_2(TMsg *pMsg)
{
    THTTPRequest *pHTTPRequest = (THTTPRequest *)pMsg;
    CHK_NULL_RETURN(pHTTPRequest);

    SessionMap::iterator itSession = m_SessionMap.find(pHTTPRequest->m_iSeq);
    if(itSession == m_SessionMap.end())
    {
        LogWarn("iSeq[%lu] is not find m_SessionMap, receive one http message:%s", pHTTPRequest->m_iSeq, pHTTPRequest->m_strRequest.c_str());
        return;
    }

    itSession->second->m_strGustIP = pHTTPRequest->m_socketHandle->GetServerIpAddress();

    LogNotice("smsService session iSeq[%lu],Guest ip[%s]!", pHTTPRequest->m_iSeq, itSession->second->m_strGustIP.data());

    string strPhone = "";
    string strContent = "";
    string strExtendPort = "";
    string strUid = "";
    string strAccount = "";
    string strPwdMd5 = "";
    string strSmsType = "";
    string strVer = "";
    string strSendTime = "";

    //unpacket
    web::HttpParams param;
    param.Parse(pHTTPRequest->m_strRequest);

    strAccount = pHTTPRequest->m_strSessionID;
    strVer = param._map["ver"];
    strPwdMd5 = param._map["password"];
    strPhone = param._map["mobile"];
    strSmsType = param._map["smstype"];
    strContent = http::UrlCode::UrlDecode(param._map["content"]);
    strSendTime = param._map["sendtime"];
    strExtendPort = param._map["extend"];
    strUid = param._map["batchid"]; 	// batchid == uid

    if(!Comm::isNumber(strExtendPort))
    {
        LogError("access httpserver ExtendPort error. Request[%s]", strExtendPort.c_str());
        proHttpsExecpt(HTTPS_RESPONSE_CODE_38, pHTTPRequest->m_iSeq);
        return;
    }
    //////////////////////////////////
    itSession->second->m_strVer = strVer;

    itSession->second->m_strUid = strUid;

    if (strPhone.empty())
    {
        LogError("strPhone[%s] or strAccount[%s] is empty!", strPhone.c_str(), strAccount.c_str());
        //// phone is empty must respone
        proHttpsExecpt(HTTPS_RESPONSE_CODE_6, pHTTPRequest->m_iSeq);
        return;
    }

    if (strAccount.empty())
    {
        LogError("strPhone[%s] or strAccount[%s] is empty!", strPhone.c_str(), strAccount.c_str());
        //// phone is empty must respone
        proHttpsExecpt(HTTPS_RESPONSE_CODE_1, pHTTPRequest->m_iSeq);
        return;
    }

    if (strSmsType.empty())
    {
        LogError("[%s:%s] strSmsType[%s] is empty!",
                 strAccount.c_str(), strPhone.c_str(),
                 strSmsType.data());
        //// phone is empty must respone
        proHttpsExecpt(HTTPS_RESPONSE_CODE_23, pHTTPRequest->m_iSeq);
        return;
    }

    // 判断短信类型
    int nSmsType = atoi(strSmsType.c_str());
    if (0 != nSmsType && 4 != nSmsType && 5 != nSmsType && 6 != nSmsType
            && 7 != nSmsType && 8 != nSmsType && 9 != nSmsType)
    {
        LogError("account[%s],strPhone[%s] smstype [%s] error!", strAccount.c_str(), strPhone.c_str(), strSmsType.c_str());
        proHttpsExecpt(HTTPS_RESPONSE_CODE_23, pHTTPRequest->m_iSeq);
        return;
    }

    ////先校验账号是否是7位
    if (6 != strAccount.length())
    {
        LogError("uid[%s] over length !", strUid.data());
        proHttpsExecpt(HTTPS_RESPONSE_CODE_1, pHTTPRequest->m_iSeq);
        return;
    }

    if (strUid.length() > 60)
    {
        LogError("uid[%s] over length !", strUid.data());
        proHttpsExecpt(HTTPS_RESPONSE_CODE_37, pHTTPRequest->m_iSeq);//-62?êy′í?ó
        return;
    }

    utils::UTFString utfHelper;
    UInt32 msgLength = utfHelper.getUtf8Length(strContent);
    UInt32 uRate = (msgLength <= 70 ? 1 : ((msgLength + 66) / 67));
    
    itSession->second->m_strVer = strVer;
    itSession->second->m_strUid = strUid;
    itSession->second->m_uFeeNum = uRate;
    itSession->second->m_uSmsfrom = SMS_FROM_ACCESS_HTTP_2;

    CHttpServerToUnityReqMsg *pHttps = new CHttpServerToUnityReqMsg();
    CHK_NULL_RETURN(pHttps);
    pHttps->m_strAccount.assign(strAccount);
    pHttps->m_strContent.assign(strContent);
    pHttps->m_strExtendPort.assign(strExtendPort);
    pHttps->m_strPwdMd5.assign(strPwdMd5);
    pHttps->m_strUid.assign(strUid);
    pHttps->m_uSmsFrom = SMS_FROM_ACCESS_HTTP_2;
    pHttps->m_strPhone.assign(strPhone);
    pHttps->m_uSessionId = pHTTPRequest->m_iSeq;
    pHttps->m_strIp = itSession->second->m_strGustIP;
    pHttps->m_strSmsType = strSmsType.c_str();
    pHttps->m_uLongSms = (uRate == 1 ? 0 : 1);
    pHttps->m_ucHttpProtocolType = pHTTPRequest->m_ucHttpProtocolType;
    pHttps->m_iMsgType = MSGTYPE_HTTPS_TO_UNITY_REQ;
    g_pUnitypThread->PostMsg(pHttps);

    return;
}

void CHttpServiceThread::HandleHTTPServerReqMsg_JD(TMsg *pMsg)
{
    if(NULL == pMsg)
    {
        LogError("invalid pMsg");

        return ;
    }
    TiXmlElement *childElement = NULL;
    std::string	strAccount;
    std::string	strPasswd;
    std::string	strMobileNums;
    std::string	strMobileCount;
    std::string	strMsg;
    std::string	strSubPort = "";
    //Uid;

    /*
    *	session check
    */
    THTTPRequest *pHTTPRequest = (THTTPRequest *)pMsg;
    SessionMap::iterator itSession = m_SessionMap.find(pHTTPRequest->m_iSeq);
    if(itSession == m_SessionMap.end())
    {
        LogWarn("iSeq[%lu] is not find m_SessionMap", pHTTPRequest->m_iSeq);

        return ;
    }
    itSession->second->m_strGustIP = pHTTPRequest->m_socketHandle->GetServerIpAddress();
    itSession->second->m_ucHttpProtocolType = HttpProtocolType_JD_Webservice;
    itSession->second->m_uSmsfrom = SMS_FROM_ACCESS_HTTP;
    itSession->second->m_strVer = "";
    itSession->second->m_strUid = "";
    LogNotice("Add smsService session iSeq[%ld],Guest ip[%s]!", pHTTPRequest->m_iSeq, itSession->second->m_strGustIP.data());

    /*
    *	parse XML message info
    */
    TiXmlDocument myDocument;
    if(myDocument.Parse(pHTTPRequest->m_strRequest.c_str(), 0, TIXML_DEFAULT_ENCODING))
    {
        LogNotice("Parse: %s faild!", pHTTPRequest->m_strRequest.c_str());
        return ;
    }

    TiXmlElement *rootElement = myDocument.RootElement();
    if(NULL == rootElement)
    {
        LogError("RootElement is NULL.")
        return ;
    }

    childElement = rootElement;
#if 1
    do
    {
        childElement = childElement->FirstChildElement();
        if(NULL == childElement)
        {
            LogError("ChildElement NULL.")
            return ;
        }
    }
    while(NULL == childElement->GetText());
#else
    do
    {
        childElement = childElement->FirstChildElement();
        if(NULL == childElement)
        {
            LogError("ChildElement NULL.")
            return ;
        }
    }
    while(0 != strcasecmp(childElement->Value(), JD_SMS_FIELD_USERID)
            && 0 != strcasecmp(childElement->Value(), JD_SMS_FIELD_PASSWD)
            && 0 != strcasecmp(childElement->Value(), JD_SMS_FIELD_MOBILENUMS)
            && 0 != strcasecmp(childElement->Value(), JD_SMS_FIELD_MOBILECOUNT)
            && 0 != strcasecmp(childElement->Value(), JD_SMS_FIELD_MSG)
            && 0 != strcasecmp(childElement->Value(), JD_SMS_FIELD_SUBPORT));
#endif

    TiXmlElement *element = childElement;
    char tag_value[64];
    do
    {
        memset(tag_value, 0, sizeof(tag_value));
        if(strlen(element->Value()) < sizeof(tag_value))
            strncpy(tag_value, element->Value(), sizeof(tag_value) - 1);
        if(0 == strcasecmp(tag_value, JD_SMS_FIELD_USERID))
        {
            strAccount = (element->GetText() == NULL ? "" : element->GetText());
        }
        else if(0 == strcasecmp(tag_value, JD_SMS_FIELD_PASSWD))
        {
            strPasswd = (element->GetText() == NULL ? "" : element->GetText());
        }
        else if(0 == strcasecmp(tag_value, JD_SMS_FIELD_MOBILENUMS))
        {
            strMobileNums = (element->GetText() == NULL ? "" : element->GetText());
        }
        else if(0 == strcasecmp(tag_value, JD_SMS_FIELD_MOBILECOUNT))
        {
            strMobileCount = (element->GetText() == NULL ? "" : element->GetText());
        }
        else if(0 == strcasecmp(tag_value, JD_SMS_FIELD_MSG))
        {
            strMsg = (element->GetText() == NULL ? "" : element->GetText());
        }
        else if(0 == strcasecmp(tag_value, JD_SMS_FIELD_SUBPORT))
        {
            strSubPort = (element->GetText() == NULL ? "" : element->GetText());
        }
        element = element->NextSiblingElement();
    }
    while(NULL != element);

    trim(strAccount);
    trim(strPasswd);
    trim(strMobileNums);
    trim(strMobileCount);
    trim(strMsg);
    trim(strSubPort);

    if (strSubPort.compare("*") == 0)
    {
        strSubPort = "";
    }


    LogDebug("Account: %s, Passwd: %s, Mobiles: %s, Mobile counts: %s, Msg: %s, SubPort: %s\n",
             strAccount.c_str(),
             strPasswd.c_str(),
             strMobileNums.c_str(),
             strMobileCount.c_str(),
             strMsg.c_str(),
             strSubPort.c_str());


    /*
     *	Mobile numbers and Account detection
    */
    if (strMobileNums.empty())
    {
        LogError("==err== strPhone[%s] or strAccount[%s] is empty!", strMobileNums.data(), strAccount.c_str());
        //// phone is empty must respone
        proHttpsExecpt_JD(NULL, HTTPS_RESPONSE_CODE_6, pHTTPRequest->m_iSeq);
        return;
    }
    int mobile_counts = atoi(strMobileCount.c_str());
    std::vector <std::string> mobileNumsList;
    Comm::split(strMobileNums, ",", mobileNumsList);
    if (mobile_counts > MAX_MOBILE_COUNTS)
    {
        LogError("invalid Mobile count: %s", strMobileCount.data());
        //// phone is empty must respone
        proHttpsExecpt_JD(RESP_CODE_JD_MOBILE_COUNTS_OVERSIZE, 0, pHTTPRequest->m_iSeq);
        return;
    }
    if (0 >= mobile_counts || mobileNumsList.size() != (unsigned int)mobile_counts)
    {
        LogError("invalid mobile count: %s or mobile nums: %s", strMobileCount.data(), strMobileNums.data());
        //// phone is empty must respone
        proHttpsExecpt_JD(RESP_CODE_JD_GENERAL_ERR, 0, pHTTPRequest->m_iSeq);
        return;
    }
    if (MAX_MOBILE_COUNTS < mobileNumsList.size())
    {
        LogError("invalid MobileNums: %s", strMobileCount.data());
        // phone is empty must respone
        proHttpsExecpt_JD(RESP_CODE_JD_MOBILE_COUNTS_OVERSIZE2, 0, pHTTPRequest->m_iSeq);
        return;
    }

    if (strAccount.empty())
    {
        LogError("==err== strPhone[%s] or strAccount[%s] is empty!", strMobileNums.data(), strAccount.c_str());
        //// phone is empty must respone
        proHttpsExecpt_JD(NULL, HTTPS_RESPONSE_CODE_1, pHTTPRequest->m_iSeq);
        return;
    }
    if (6 != strAccount.length())
    {
        LogError("==err== account[%s] dose exist!", strAccount.c_str());
        proHttpsExecpt_JD(NULL, HTTPS_RESPONSE_CODE_1, pHTTPRequest->m_iSeq);
        return;
    }

    utils::UTFString utfHelper;
    UInt32 msgLength = utfHelper.getUtf8Length(strMsg);
    UInt32 uRate = (msgLength <= 70) ? 1 : ((msgLength + 66) / 67);

    itSession->second->m_uFeeNum = uRate;
    //itSession->second->m_uSmsfrom = SMS_FROM_ACCESS_HTTP_3;

    /*
    *	build the request and send it
    */
    CHttpServerToUnityReqMsg *pHttps = new CHttpServerToUnityReqMsg();
    pHttps->m_strAccount.assign(strAccount);
    pHttps->m_strContent.assign(strMsg);
    pHttps->m_strExtendPort.assign( strSubPort );		//no extend port
    pHttps->m_strPwdMd5.assign(strPasswd);
    pHttps->m_strUid = "";
    pHttps->m_uSmsFrom = SMS_FROM_ACCESS_HTTP;		//SMS_FROM_ACCESS_HTTP_3
    pHttps->m_strPhone.assign(strMobileNums);
    pHttps->m_uSessionId = pHTTPRequest->m_iSeq;
    pHttps->m_strIp = itSession->second->m_strGustIP;
    pHttps->m_strSmsType = "0";				//Notification type default
    pHttps->m_uLongSms = (uRate == 1 ? 0 : 1);
    pHttps->m_iMsgType = MSGTYPE_HTTPS_TO_UNITY_REQ;
    pHttps->m_ucHttpProtocolType = itSession->second->m_ucHttpProtocolType;

    g_pUnitypThread->PostMsg(pHttps);
}

void CHttpServiceThread::HandleHTTPServerTemplateReqMsg(TMsg *pMsg)
{
    THTTPRequest *pHTTPRequest = (THTTPRequest *)pMsg;
    CHK_NULL_RETURN(pHTTPRequest);

    SessionMap::iterator itSession = m_SessionMap.find(pHTTPRequest->m_iSeq);
    if(itSession == m_SessionMap.end())
    {
        LogWarn("iSeq[%lu] is not find m_SessionMap, receive one http message:%s.", pHTTPRequest->m_iSeq, pHTTPRequest->m_strRequest.c_str());
        return;
    }

    itSession->second->m_strGustIP = pHTTPRequest->m_socketHandle->GetServerIpAddress();

    LogNotice("smsService session iSeq[%lu], Guest ip[%s]!", pHTTPRequest->m_iSeq, itSession->second->m_strGustIP.c_str());

    std::string strAccount = "";
    std::string strPwdMd5 = "";
    std::string strPhone = "";
    std::string strTemplateid = "";
    std::string strParam = "";
    std::string strExtendPort = "";
    std::string strUid = "";
    std::string strSmsType = "";

    //unpacket
    try
    {
        Json::Reader reader(Json::Features::strictMode());
        Json::Value root;
        std::string js = "";

        if (json_format_string(pHTTPRequest->m_strRequest, js) < 0)
        {
            LogError("json message error, req[%s]", pHTTPRequest->m_strRequest.data());
            //// failed reponse
            proHttpsExecpt(HTTPS_RESPONSE_CODE_20, pHTTPRequest->m_iSeq);
            return;
        }
        if(!reader.parse(js, root))
        {
            LogError("json parse is failed, req[%s]", pHTTPRequest->m_strRequest.data());
            //// failed reponse
            proHttpsExecpt(HTTPS_RESPONSE_CODE_21, pHTTPRequest->m_iSeq);
            return;
        }

        strAccount = root["clientid"].asString();
        strPwdMd5 = root["password"].asString();
        strPhone = root["mobile"].asString();
        strTemplateid = root["templateid"].asString();
        strParam = root["param"].asString();
        strExtendPort = root["extend"].asString();
        strUid = root["uid"].asString();

        //strParam不用去空格
        Comm::trim(strAccount);
        Comm::trim(strPwdMd5);
        Comm::trim(strPhone);
        Comm::trim(strTemplateid);
        Comm::trim(strExtendPort);

        if(!Comm::isNumber(strExtendPort))
        {
            LogError("[%s:%s] access httpserver ExtendPort error. Request[%s]",
                     strAccount.c_str(), strPhone.c_str(),
                     strExtendPort.c_str());
            proHttpsExecpt(HTTPS_RESPONSE_CODE_38, pHTTPRequest->m_iSeq);
            return;
        }
    }
    catch(...)
    {
        LogError("try catch access httpserver unpacet error, json err. Request[%s]", pHTTPRequest->m_strRequest.data());
        //// failed reponse
        proHttpsExecpt(HTTPS_RESPONSE_CODE_21, pHTTPRequest->m_iSeq);
        return;
    }

    SMSSmsInfo smsInfo;
    smsInfo.m_strClientId = strAccount;
    smsInfo.m_strPhone = strPhone;
    smsInfo.m_strTemplateId = strTemplateid;
    smsInfo.m_strTemplateParam = strParam;
    smsInfo.m_strExtpendPort = strExtendPort;
    smsInfo.m_strUid = strUid;

    itSession->second->m_strUid = strUid;

    if (strAccount.empty())
    {
        LogError("param error: strAccount is empty");
        // phone is empty must respone
        proHttpsExecpt(HTTPS_RESPONSE_CODE_1, pHTTPRequest->m_iSeq);
        return;
    }

    if (strPwdMd5.empty())
    {
        LogError("param error: account:%s, strPwdMd5 is empty!", strAccount.c_str());
        // phone is empty must respone
        proHttpsExecpt(HTTPS_RESPONSE_CODE_1, pHTTPRequest->m_iSeq);
        return;
    }

    if (strPhone.empty())
    {
        LogError("param error: account:%s, strPhone is empty!", strAccount.c_str());
        // phone is empty must respone
        proHttpsExecpt(HTTPS_RESPONSE_CODE_6, pHTTPRequest->m_iSeq);
        return;
    }

    if (strUid.length() > 60)
    {
        LogError("account:%s, uid[%s] over length !", strAccount.c_str(), strUid.data());
        proHttpsExecpt(HTTPS_RESPONSE_CODE_37, pHTTPRequest->m_iSeq);
        return;
    }

    if (strTemplateid.empty())
    {
        LogError("account:%s, templateid is null!", strAccount.c_str());
        proHttpsExecpt(HTTPS_RESPONSE_CODE_18, pHTTPRequest->m_iSeq);

        smsInfo.m_strErrorCode = ERRORCODE_TEMPLATEID_IS_NULL;
        InsertAccessDb(smsInfo);
        return;
    }

    std::string::size_type templateParaLength = strParam.length();

    if (TEMPLATE_PARAMETER_MAX_LEN < templateParaLength)
    {
        LogError("templatePara[%u, %s] is too long!", templateParaLength, strParam.c_str());

        // Truncation is required to prevent the failure of inserting the database.
        smsInfo.m_strTemplateParam = strParam.substr(0, TEMPLATE_PARAMETER_MAX_LEN);
        smsInfo.m_strErrorCode = ERRORCODE_TEMPLATE_PARAM_ERROR;

        proHttpsExecpt(HTTPS_RESPONSE_CODE_19, pHTTPRequest->m_iSeq);
        InsertAccessDb(smsInfo);
        return;
    }

    if ((strParam.find("{") != std::string::npos)
            || (strParam.find("}") != std::string::npos)
            || (strParam.find("#*#") != std::string::npos))
    {
        LogError("param[%s] shoul not include '{' or '}' or \"#*#\"!", strParam.c_str());
        proHttpsExecpt(HTTPS_RESPONSE_CODE_19, pHTTPRequest->m_iSeq);

        smsInfo.m_strErrorCode = ERRORCODE_TEMPLATE_PARAM_ERROR;
        InsertAccessDb(smsInfo);
        return;
    }

    //先校验账号是否存在
    AccountMap::iterator accountItor = m_mapAccount.find(strAccount);
    if (accountItor == m_mapAccount.end())
    {
        LogError("account[%s] dose exist!", strAccount.c_str());
        proHttpsExecpt(HTTPS_RESPONSE_CODE_1, pHTTPRequest->m_iSeq);
        return;
    }

    std::string strkey = "";
    strkey.assign(strAccount).append("_").append(strTemplateid);
    SmsTemplateMap::iterator itor = m_mapSmsTemplate.find(strkey);
    if (itor == m_mapSmsTemplate.end())
    {
        LogError("account[%s] has no available sms_template[%s]!", strAccount.c_str(), strTemplateid.c_str());
        proHttpsExecpt(HTTPS_RESPONSE_CODE_15, pHTTPRequest->m_iSeq);

        smsInfo.m_strErrorCode = ERRORCODE_TEMPLATEID_NOEXIST;
        InsertAccessDb(smsInfo);
        return;
    }

    std::string strType = itor->second.m_strType;

    strSmsType = (atoi(strType.c_str()) == TEMPLATE_TYPE_HANG_UP_SMS) ?  itor->second.m_strSmsType : strType;
    if (strSmsType.empty())
    {
        LogError("[%s:%s] strSmsType[%s] is empty!", strAccount.c_str(), strPhone.c_str(), strSmsType.c_str());
        //// phone is empty must respone
        proHttpsExecpt(HTTPS_RESPONSE_CODE_23, pHTTPRequest->m_iSeq);
        return;
    }

    // 判断模板短信类型
    int nSmsType = atoi(strSmsType.c_str());
    if (0 != nSmsType && 4 != nSmsType && 5 != nSmsType && 6 != nSmsType
            && 7 != nSmsType && 8 != nSmsType && 9 != nSmsType)
    {
        LogError("account[%s],strPhone[%s] smstype [%s] error!", strAccount.c_str(), strPhone.c_str(), strSmsType.c_str());
        proHttpsExecpt(HTTPS_RESPONSE_CODE_23, pHTTPRequest->m_iSeq);
        return;
    }

    //校验模板参数个数和模板参数内容长度
    const char *szReg = "(\\{.*?\\})";
    std::string strContent = itor->second.m_strContent;    //防止改变模板本身

    boost::smatch what;
    boost::regex expression(szReg);

    std::list<std::string> matchResultList;    //保存匹配到的占位符

    //不知道有多少个占位符，所以这里先保存所有占位符
    std::string::const_iterator start = strContent.begin();
    std::string::const_iterator end = strContent.end();
    while (boost::regex_search(start, end, what, expression))
    {
        matchResultList.push_back(what[0]);
        start = what[0].second;
    }

    std::vector<std::string> vecParam;
    Comm::splitExVector(strParam, ";", vecParam);

    //参数个数检验
    if (vecParam.size() != matchResultList.size())
    {
        LogError("[%s:%s] params[%s] is not match template[%s]!",
                 strAccount.c_str(), strPhone.c_str(), strParam.c_str(), strContent.c_str());
        proHttpsExecpt(HTTPS_RESPONSE_CODE_16, pHTTPRequest->m_iSeq);

        smsInfo.m_strErrorCode = ERRORCODE_TEMPLATE_PARAM_ERROR;
        smsInfo.m_strSmsType = strSmsType;
        InsertAccessDb(smsInfo);
        return;
    }

    {
        int i = 0;
        for (std::list<std::string>::iterator itor = matchResultList.begin(); itor != matchResultList.end(); ++itor)
        {
            boost::replace_first(strContent, *itor, vecParam.at(i++));
        }
    }

    std::string strLeft = "[";
    std::string strRight = "]";
    if (true == Comm::IncludeChinese((char *)strContent.c_str()) ||
            true == Comm::IncludeChinese((char *)itor->second.m_strSign.c_str()))
    {
        strLeft = "%e3%80%90";
        strRight = "%e3%80%91";
        strLeft = http::UrlCode::UrlDecode(strLeft);
        strRight = http::UrlCode::UrlDecode(strRight);
    }

    strContent = strLeft + itor->second.m_strSign + strRight + strContent;   //组合短信
    LogDebug("join over, get strContent[%s]", strContent.c_str());

    utils::UTFString utfHelper;
    UInt32 msgLength = utfHelper.getUtf8Length(strContent);
    UInt32 uRate = (msgLength <= 70 ? 1 : ((msgLength + 66) / 67));

    itSession->second->m_uFeeNum = uRate;
    itSession->second->m_uSmsfrom = SMS_FROM_ACCESS_HTTP;

    smsInfo.m_uLongSms = (uRate == 1 ? 0 : 1);

    CHttpServerToUnityReqMsg *pHttps = new CHttpServerToUnityReqMsg();
    CHK_NULL_RETURN(pHttps);

    pHttps->m_strAccount.assign(strAccount);
    pHttps->m_strContent.assign(strContent);
    pHttps->m_strExtendPort.assign(strExtendPort);
    pHttps->m_strPwdMd5.assign(strPwdMd5);
    pHttps->m_strUid.assign(strUid);
    pHttps->m_uSmsFrom = SMS_FROM_ACCESS_HTTP;
    pHttps->m_strPhone.assign(strPhone);
    pHttps->m_uSessionId = pHTTPRequest->m_iSeq;
    pHttps->m_strIp = itSession->second->m_strGustIP;
    pHttps->m_strSmsType = strSmsType.data();
    pHttps->m_strTemplateId = strTemplateid;
    pHttps->m_strTemplateParam = strParam;
    pHttps->m_strTemplateType = strType;
    pHttps->m_uLongSms = smsInfo.m_uLongSms;
    pHttps->m_ucHttpProtocolType = pHTTPRequest->m_ucHttpProtocolType;
    pHttps->m_iMsgType = MSGTYPE_HTTPS_TO_UNITY_REQ;
    g_pUnitypThread->PostMsg(pHttps);

    return;
}

void CHttpServiceThread::InsertAccessDb(SMSSmsInfo smsInfo)
{
    struct tm timenow = {0};
    time_t now = time(NULL);
    char submitTime[64] = { 0 };
    localtime_r(&now, &timenow);
    strftime(submitTime, sizeof(submitTime), "%Y%m%d%H%M%S", &timenow);

    char strDate[32] = {0};
    strftime(strDate, sizeof(strDate), "%Y%m%d", &timenow);

    UInt32 uIdentify = 0;
    AccountMap::iterator accountItor = m_mapAccount.find(smsInfo.m_strClientId);
    if (accountItor != m_mapAccount.end())
    {
        uIdentify = accountItor->second.m_uIdentify;
    }

    string strTempDesc = "";
    map<string, string>::iterator itrDesc = m_mapSystemErrorCode.find(smsInfo.m_strErrorCode);
    if (itrDesc != m_mapSystemErrorCode.end())
    {
        strTempDesc.assign(smsInfo.m_strErrorCode);
        strTempDesc.append("*");
        strTempDesc.append(itrDesc->second);
    }
    else
    {
        strTempDesc.assign(smsInfo.m_strErrorCode);
    }

    Comm::split(smsInfo.m_strPhone, "," , smsInfo.m_Phonelist);
    UInt32  uPhoneCount = smsInfo.m_Phonelist.size();

    smsInfo.m_strDate.assign(submitTime);
    smsInfo.m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
    smsInfo.m_strSmsId = getUUID();

    MYSQL *pMysqlConn = g_pDisPathDBThreadPool->CDBGetConn();

    for(UInt32 i = 0; i < uPhoneCount; i++)
    {
        std::string strTmpId = getUUID();

        char szSql[10240] = {0};
        char szContent[2048] = {0};
        char szTempParam[2048] = {0};
        char szSign[128] = {0};
        char szUid[64] = {0};

        UInt32 position = smsInfo.m_strSign.length();
        if(smsInfo.m_strSign.length() > 100)
        {
            position = Comm::getSubStr(smsInfo.m_strSign, 100);
        }
        
        if(pMysqlConn != NULL)
        {
            mysql_real_escape_string(pMysqlConn, szContent, smsInfo.m_strContent.data(), smsInfo.m_strContent.length());
            mysql_real_escape_string(pMysqlConn, szSign, smsInfo.m_strSign.substr(0, position).data(), smsInfo.m_strSign.substr(0, position).length());
            mysql_real_escape_string(pMysqlConn, szUid, smsInfo.m_strUid.substr(0, 60).data(), smsInfo.m_strUid.substr(0, 60).length());
            if (!smsInfo.m_strTemplateParam.empty())
            {
                mysql_real_escape_string(pMysqlConn, szTempParam, smsInfo.m_strTemplateParam.data(),
                                         smsInfo.m_strTemplateParam.length());
            }
        }

        snprintf(szSql, 10240, "insert into t_sms_access_%d_%s(id,content,srcphone,phone,smscnt,smsindex,sign,submitid,smsid,clientid,"
                 "operatorstype,smsfrom,state,errorcode,date,innerErrorcode,channelid,smstype,charge_num,paytype,agent_id,username ,isoverratecharge,"
                 "uid,showsigntype,product_type,c2s_id,process_times,longsms,channelcnt,template_id,temp_params,sid)"
                 "  values('%s', '%s', '%s', '%s', '%d', '%d', '%s', '%ld', '%s', '%s', '%d',"
                 "'%d', '%d', '%s', '%s','%s','%d','%d','%d','%d','%ld','%s','%d','%s','%d','%d','%lu','%d','%d','%d', %s, '%s', '%s');",
                 uIdentify,
                 strDate,
                 strTmpId.c_str(),
                 szContent,
                 smsInfo.m_strSrcPhone.substr(0, 20).data(),
                 smsInfo.m_Phonelist[i].substr(0, 20).data(),
                 1,
                 1,
                 szSign,
                 smsInfo.m_uSubmitId,
                 smsInfo.m_strSmsId.data(),
                 smsInfo.m_strClientId.data(),
                 smsInfo.m_uOperater,
                 smsInfo.m_uSmsFrom,
                 smsInfo.m_uState,
                 strTempDesc.data(),
                 smsInfo.m_strDate.data(),
                 strTempDesc.data(),
                 0,
                 atoi(smsInfo.m_strSmsType.data()),
                 smsInfo.m_uSmsNum,
                 smsInfo.m_uPayType,
                 smsInfo.m_uAgentId,
                 smsInfo.m_strUserName.data(),
                 smsInfo.m_uOverRateCharge,
                 szUid,
                 smsInfo.m_uShowSignType,
                 smsInfo.m_uProductType,
                 g_uComponentId,
                 smsInfo.m_uProcessTimes,
                 smsInfo.m_uLongSms,
                 1,
                 (smsInfo.m_strTemplateId.empty()) ? "NULL" : smsInfo.m_strTemplateId.data(),
                 szTempParam,
                 " ");

        LogDebug("[%s: ] access insert DB sql[%s].", smsInfo.m_strSmsId.data(), szSql);

        TMQPublishReqMsg *pMQ = new TMQPublishReqMsg();
        CHK_NULL_RETURN(pMQ);

        pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
        pMQ->m_strExchange.assign(g_strMQDBExchange);
        pMQ->m_strRoutingKey.assign(g_strMQDBRoutingKey);
        pMQ->m_strData.assign(szSql);
        pMQ->m_strData.append("RabbitMQFlag=");
        pMQ->m_strData.append(strTmpId.c_str());
        g_pMQDBProducerThread->PostMsg(pMQ);
    }

    return;
}

