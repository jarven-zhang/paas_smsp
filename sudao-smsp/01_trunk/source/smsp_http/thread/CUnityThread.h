#ifndef __CUNITYTHREAD__
#define __CUNITYTHREAD__

#include "Thread.h"
#include "boost/regex.hpp"
#include "boost/shared_ptr.hpp"
#include "SMSSmsInfo.h"
#include "MQWeightGroup.h"
#include "PhoneDao.h"
#include "SnManager.h"

using namespace boost;

#define ACCOUNT_UNLOCK_TIMER_ID                 1
#define TIMERSEND_TASK_INSTALL_TIMER_ID         2         // 定时短信任务定时运行时设置的定时器
#define TIMERSEND_TASKS_START_EXECUTE_TIMER_ID  3         // 组件启动时设置开始执行定时任务的定时器
const int TIMERSEND_TASKS_START_EXEUCTE_DELAY = 20;        // 组件启动时开始执行定时任务的时间
#define TIMERSEND_TASKS_PROGRESS_ERASE_TIMER_ID  4        // 定时短信进度缓存清除的定时器
const int TIMERSEND_TASKS_PROGRESS_CACHE_TIMEOUT = 60 * 60;     // 定时短信进度缓存超时秒数
const int TIMERSEND_TASKS_PROGRESS_CACHE_CLEANER_INTERVAL = 60 * 60;     // 定时短信进度缓存清除之间的间隔秒数
#define INVISIBLE_NUMBER_STRING "00000000000"

extern const int HTTPS_RESPONSE_CODE_0;
extern const int HTTPS_RESPONSE_CODE_1;
extern const int HTTPS_RESPONSE_CODE_2;
extern const int HTTPS_RESPONSE_CODE_3;
extern const int HTTPS_RESPONSE_CODE_4;
extern const int HTTPS_RESPONSE_CODE_5;
extern const int HTTPS_RESPONSE_CODE_6;
extern const int HTTPS_RESPONSE_CODE_7;
extern const int HTTPS_RESPONSE_CODE_8;
extern const int HTTPS_RESPONSE_CODE_9;
extern const int HTTPS_RESPONSE_CODE_10;
extern const int HTTPS_RESPONSE_CODE_11;
extern const int HTTPS_RESPONSE_CODE_12;
extern const int HTTPS_RESPONSE_CODE_13;
extern const int HTTPS_RESPONSE_CODE_14;
extern const int HTTPS_RESPONSE_CODE_15;
extern const int HTTPS_RESPONSE_CODE_16;
extern const int HTTPS_RESPONSE_CODE_17;
extern const int HTTPS_RESPONSE_CODE_18;
extern const int HTTPS_RESPONSE_CODE_19;
extern const int HTTPS_RESPONSE_CODE_20;
extern const int HTTPS_RESPONSE_CODE_21;
extern const int HTTPS_RESPONSE_CODE_22;
extern const int HTTPS_RESPONSE_CODE_23;
extern const int HTTPS_RESPONSE_CODE_24;
extern const int HTTPS_RESPONSE_CODE_25;
extern const int HTTPS_RESPONSE_CODE_26;
extern const int HTTPS_RESPONSE_CODE_27;
extern const int HTTPS_RESPONSE_CODE_28;
extern const int HTTPS_RESPONSE_CODE_29;
extern const int HTTPS_RESPONSE_CODE_30;
extern const int HTTPS_RESPONSE_CODE_31;

const UInt32 TimerSendTaskState_PreHanding = 0;
const UInt32 TimerSendTaskState_WaitingSend = 1;
const UInt32 TimerSendTaskState_Sending = 2;
const UInt32 TimerSendTaskState_Sent= 3;
const UInt32 TimerSendTaskState_Cancled = 4;
const UInt32 TimerSendTaskState_Deleted = 5;
const UInt32 TimerSendTaskState_Failed = 6;

const UInt32 TimerSendPhoneState_AcceptSucc = 1;
const UInt32 TimerSendPhoneState_AcceptFail = 2;
const UInt32 TimerSendPhoneState_Handled = 3;

class CHttpServerToUnityReqMsg:public TMsg
{
public:
    CHttpServerToUnityReqMsg()
    {
        m_uSmsFrom = 0;
        m_uSessionId = 0;
        m_uLongSms = 0;
        m_ucHttpProtocolType = INVALID_UINT8;
        m_bTimerSend = false;
    }

    string m_strPhone;
    string m_strContent;
    string m_strAccount;
    string m_strPwdMd5;
    string m_strUid;
    string m_strExtendPort;
    string m_strIp;
    UInt32 m_uSmsFrom;
    UInt64 m_uSessionId;
    string m_strSmsType;
    UInt32 m_uLongSms;
    string m_strChannelId;
    //模板短信所需参数
    string m_strTemplateId;
    string m_strTemplateParam;
    string m_strTemplateType;

    UInt8 m_ucHttpProtocolType;

    bool m_bTimerSend;
    string m_strTimerSendTime;
    UInt8 m_uTimerSendSubmittype;
};

class CUnityToHttpServerRespMsg:public TMsg
{
public:
    string m_strSmsid;
    UInt64 m_uSessionId;
    vector<string> m_vecCorrectPhone;
    vector<string> m_vecFarmatErrorPhone;
    vector<string> m_vecParamErrorPhone;
    vector<string> m_vecNomatchErrorPhone;
    vector<string> m_vecBlackListPhone;
    vector<string> m_vecRepeatPhone;
    map<string, string> m_mapPhoneSmsId;
    Int32 m_iErrorCode;  //// this is m_vecParamErrorPhone
    string m_strError;    //// this is m_vecParamErrorPhone

    string m_strUserResp;
    string m_strDisplayNum; // for tencent cloud "display_number"
    bool m_bTimerSend;
};

struct StProgress
{
    StProgress() : uCurIndx(0) {}
    UInt32 uCurIndx;
    UInt32 uTotal;
    time_t createTime;
};

typedef boost::shared_ptr<CThreadWheelTimer> thread_wheeltimer_ptr_t;
struct StTimerSendTaskInfo
{
    StTimerSendTaskInfo()
        : uChargeNum(1),
        uTotalPhonesNum(0),
        uHandledPhonesNum(0),
        uTotalPhonesNumInBatch(0),
        uHandledPhonesNumInBatch(0),
        bSmsInfoLoaded(false)
    {
    }

    std::string strTaskUuid;
    std::string strSmsId;
    std::string strClientId;
    std::string strContent;
    std::string strSign;
    std::string strExtendPort;
    std::string strSrcPhone;
    std::string strUid;
    UInt64 uSmsType;
    UInt32 uSmsFrom;
    UInt32 uIdentify;
    std::string strSubmitTime;
    std::string strSetSendTime;
    std::string strPhonesTblIdx;

    UInt32 uChargeNum;

    thread_wheeltimer_ptr_t taskTimerPtr;

    UInt32 uTotalPhonesNum;   // 任务接收成功，即应该发送的号码总数
    UInt32 uHandledPhonesNum;    // 按批取号码时已取且处理的号码数
    UInt32 uStatus;

    UInt32 uTotalPhonesNumInBatch;   // 本批次取的号码总数
    UInt32 uHandledPhonesNumInBatch;    // 本批次取的号码中已经处理的号码

    bool bSmsInfoLoaded;

    std::string strSignPort;

    /*------ SmsInfo 相关 -------*/
    std::string strDeliveryUrl;
    std::string strUserName;
    UInt32 uShowSignType;
    bool bIsChinese;
    std::string strSid;
    UInt32 uNeedExtend;
    UInt32 uAgentId;
    UInt32 uNeedSignExtend;
    UInt32 uNeedAudit;
    UInt32 uPayType;
    UInt32 uOverRateCharge;
    UInt32 uBelong_sale;
};

typedef boost::shared_ptr<StTimerSendTaskInfo> timersend_taskinfo_ptr_t;

class CUnityThread:public CThread
{
    typedef std::map<string, SmsAccount> AccountMap;
    typedef std::map<std::string,ClientIdSignPort> ClientIdAndSignMap;
    public:
    CUnityThread(const char* name);
    virtual ~CUnityThread();
    bool Init(UInt64 SwitchNumber);
    UInt32 GetSessionMapSize();

    private:
    void HandleMsg(TMsg* pMsg);

    void MainLoop();

    void HandleDBResp(TMsg* pMsg);
    void HandleTimerSendTaskQueryDBResp(TMsg* pMsg);
    void HandleTimerSendTaskUpdateDBResp(TMsg* pMsg);
    void UpdateTimerSendTaskPhonesDBStateAndExtra(const std::string& strID,
                                                  const std::string& strTaskUuid,
                                                  int iNewState,
                                                  const std::string& strExtraSql = "");
    void UpdateTimerSendTaskDBStateAndExtra(const std::string& strTaskUuid,
                                            const std::string& strStateIdentifier,
                                            int iNewState,
                                            const std::string& strExtraSql = "");
    /* 取消定时任务，并清除任务信息缓存 */
    void CancelAndEreaseTask(const std::string& strTaskUuid);
    /* 处理新取出的一批号码
     * 主要是组包发送到MQ，然后再取下一批号码
     * */
    void HandleNewFetchedPhones(const std::string& strSessTaskUuid, RecordSet *pRs);

    bool GetTimersendTaskPhonesTblIdx(const std::string& strTaskUuid, std::string& strTblIdx);

    bool CheckAndLoadSmsInfoByTaskInfo(StTimerSendTaskInfo& taskInfo);
    bool PacketSmsAndSend2MQ(StTimerSendTaskInfo& stTaskInfo, const std::vector<std::string>& vPhoneList);
    void FetchNextBatchTimesendPhonesFromDB(const std::string& strTaskUuid);
    /**
      process table t_sms_account update
      */
    void HandleAccountUpdateReq(TMsg* pMsg);

    /**
      process account 5 failed locked
      */
    void AccountLock(SmsAccount& accountInfo, const char* accountLockReason);
    /**
      process smpp or cmpp insert access db
      */
    void InsertAccessDb(SMSSmsInfo* pSmsInfo);


    /**
      process timer send sms db record insert
      */
    void InsertTimerSendSmsDB(SMSSmsInfo* pSmsInfo,
            CHttpServerToUnityReqMsg* pHttps,
            const vector<string>& vecErrorPhone,
            const vector<string>& vecNomatchPhone);


    bool checkPhoneFormat(SMSSmsInfo* pSmsInfo,
            vector<string>& formatErrorVec,
            vector<string>& nomatchVec);

    /**
return: true 检查成功，false检查失败
*/
    bool CheckSignExtend(SMSSmsInfo* pSmsInfo);

    /**
      from content extract sign and content,showsigntype
0:ok
-6:缺少参数
-8:签名过长或者过短
-9:签名未报备
*/
    int ExtractSignAndContent(const string& strContent,string& strOut,string& strSign,UInt32& uChina,UInt32& uShowSignType);

    /**
return: true表示获取签名端口成功，false获取签名端口失败
*/
    bool getSignPort(const string& strClientId,string& strSign, string& strOut);

    /**
return: true 表示检查成功，false表示检查失败
*/
    bool checkSignAndContent(const string& strSign, const string& strContent);

    /**
      process timeout msg
      */
    void HandleTimeOut(TMsg* pMsg);

    /**
      process httpserver send to msg
      */
    void HandleHttpsReqMsg(TMsg* pMsg);

    /**
      process check https signport/phone numbers/sign length/sign
return:true check paas, false check not pass
*/
    bool CheckHttpsProcess(SMSSmsInfo* pSmsInfo,Int32& iErrorCode, string& strError,string& strUserResp);

    /**
      process set redis accesssms:smsid_phone
      */
    void proSetInfoToRedis(SMSSmsInfo* pSmsInfo);

    /**
     * Handle new added timersend sms request
     */
    void handleNewTimersendTask(const std::string& strTaskUuid, timersend_taskinfo_ptr_t pNewTaskPtr);

    void executeTimersendTask(const std::string& strTaskUuid);
    /*
       IOMQ link err, and g_pIOMQPublishThread.queuesize > m_uIOProduceMaxChcheSize.
       then will update db and send report.
       */
    void UpdateRecord(SMSSmsInfo* pInfo);
    bool signalErrSmsPushRtAndDb(SMSSmsInfo* pSmsInfo,UInt32 i);
    bool signalErrSmsPushRtAndDb2(SMSSmsInfo* pSmsInfo,UInt32 i);
    void GetSysPara(const std::map<std::string, std::string>& mapSysPara);
    bool updateMQWeightGroupInfo(const map<string,ComponentRefMq>& componentRefMqInfo);

    bool checkAccount(const CHttpServerToUnityReqMsg* const pHttps,
            const SMSSmsInfo* const pSmsInfo,
            AccountMap::iterator& iterAccount);

    bool checkAccountIp(const CHttpServerToUnityReqMsg* const pHttps,
            const SMSSmsInfo* const pSmsInfo,
            const AccountMap::iterator& iterAccount);

    bool checkAccountPwd(const CHttpServerToUnityReqMsg* const pHttps,
            const SMSSmsInfo* const pSmsInfo,
            const AccountMap::iterator& iterAccount);

    bool checkProtocolType(const CHttpServerToUnityReqMsg* const pHttps,
            SMSSmsInfo* const pSmsInfo,
            const AccountMap::iterator& iterAccount);

    bool checkContent(const CHttpServerToUnityReqMsg* const pHttps,
            SMSSmsInfo* const pSmsInfo);

    bool checkSignExtend(const CHttpServerToUnityReqMsg* const pHttps,
            SMSSmsInfo* const pSmsInfo);

    bool checkPhoneList(const CHttpServerToUnityReqMsg* const pHttps,
            const SMSSmsInfo* const pSmsInfo);

    void rsponseToClient(const CHttpServerToUnityReqMsg* const pHttps,
            const SMSSmsInfo* const pSmsInfo,
            const int iErrorCode,
            const string& strError);

    void generateSmsIdForEveryPhone(SMSSmsInfo* const pSmsInfo);
    const string& getSmsId(const SMSSmsInfo* const pSmsInfo, const string& strPhone);

    bool getPortSign( string strClientId, string strSignPort, string &strSignOut );
    bool getContentWithSign( SMSSmsInfo* pSmsInfo, string& strSign, UInt32& uiChina );

    void storeTimerTaskInfo();

    // 更新定时发送短信任务信息
    bool updateTimersendTaskInfo(const std::map<std::string, timersend_taskinfo_ptr_t>& m_mapTimersendTaskInfo);

    private:
    AccountMap m_AccountMap;
    set<string> m_TestAccount;//account only for test
    SnManager m_SnManager;
    PhoneDao m_honeDao;
    UInt64 m_uSwitchNumber;
    ClientIdAndSignMap m_mapClientSignPort;
    CThreadWheelTimer* m_UnlockTimer;

    CThreadWheelTimer* m_LinkNumTimer;//for account's LinkNum to db

    std::map<std::string, models::UserGw> m_userGwMap; //KEY: userid_smstype

    //MQ config
    UInt32 m_uSelectChannelMaxNum;


    UInt32 m_uIOProduceMaxChcheSize;    //IF IOMQ error, g_pMQIOProducerThread will cache m_uIOProduceMaxChcheSize at most

    UInt32 m_uC2sId;    //c2s组件ID

    ///smsp_v5.0
    map<UInt32,MiddleWareConfig> m_middleWareConfigInfo;
    ComponentConfig m_componentConfigInfo;
    map<string,ListenPort> m_listenPortInfo;
    map<UInt64,MqConfig> m_mqConfigInfo;
    map<string,ComponentRefMq> m_componentRefMqInfo;
    set<UInt64> m_mqId;     ///all of this c2s's mq

    map<string,string> m_mapSystemErrorCode;

    // ---- Timsersend task ----
    std::map<string, StProgress> m_timerSndPhonesInsrtProgressCache;
    UInt32 m_uTimersendTaskPrefetchPhonesCnt;
    std::map<std::string, timersend_taskinfo_ptr_t> m_mapTimersendTaskInfo;
    // 如果启动时直接读取任务可能会直接执行到期的任务，而其它线程还没启动好
    // 因此使用定时器，在隔一段时间后再开始读取任务
    CThreadWheelTimer* m_TimersendTaskStartExecuteTimer;
    CThreadWheelTimer* m_TimersendTaskProgressCleanTimer;
    bool m_bTimersendTaskStartExecution;

    mq_weightgroups_map_t m_mapMQWeightGroups;
    ////tps and hold
    //string m_strTpsHoldExchange;
    //string m_strTpsHoldRoutingKey;
};

extern CUnityThread                    *g_pUnitypThread;

#endif ////__CUNITYTHREAD__
