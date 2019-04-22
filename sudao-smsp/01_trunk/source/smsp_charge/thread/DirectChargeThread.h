#ifndef THREAD_DIRECTCHARGETHREAD_H
#define THREAD_DIRECTCHARGETHREAD_H

#include "platform.h"
#include "Thread.h"
#include "AppData.h"
#include "SnManager.h"


class DirectChargeReq: public TMsg
{
public:
    DirectChargeReq()
    {
        m_uiAgentType = 0;
        m_uiPayType = 0;
        m_uiOperater = 0;
        m_uiArea = 0;
        m_uiCount = 0;
        m_dFee = 0;
        m_uiClientAscription = 0;
    }

    string m_strClientID;
    UInt32 m_uiAgentType;    //1:销售代理商,2:品牌代理商,3:资源合作商,4:代理商和资源合作,5:OEM代理商
    string m_strAgendId;
    UInt32 m_uiPayType;
    string m_strProductType;  //多个产品类型之间以英文逗号间隔
    UInt32 m_uiOperater; //运营商，0全网 1移动 2联通 3电信 4国际
    UInt32 m_uiArea;     //对应t_sms_area表中一级area_id(省)
    UInt32 m_uiCount;
    double m_dFee;
    string m_strSmsID;
    string m_strPhone;
    UInt32 m_uiClientAscription; // 0 = a li,1 = dai li shang ,2 = yun ping tai
};


class DirectChargeThread : public CThread
{
struct SmsSalePrice
{
    string m_strDate;
    double m_dPrice;
};

class Session
{
public:
    enum ChargeType
    {
        ChargeType_Init,
        ChargeType_GivePool,
        ChargeType_Balance,
    };

public:
    Session(DirectChargeThread* pThread, UInt64 ulSequence=0);
    ~Session();

    void setReq(const DirectChargeReq& req) {m_req = req;}

private:
    Session(const Session&);
    Session& operator=(const Session&);

public:
    DirectChargeThread* m_pThread;

    UInt64 m_ulSequence;

    ChargeType m_eChargeType;

    // 扣赠送短信成功后，更新内存
    TSmsDirectClientGivePool* m_pGivePool;

    DirectChargeReq m_req;

    UInt32 m_uiRetCode;

    // 订单id
    string m_strOrderId;

    // 产品销售价，针对客户
    double m_dSalePrice;

    // 产品成功(元)，针对代理商
    double m_dProductCost;
};

typedef map<UInt64, Session*> SessionMap;
typedef SessionMap::iterator SessionMapIter;

typedef vector<SmsSalePrice> SmsSalePriceVec;
typedef map<string, SmsSalePriceVec > SmsSalePriceMap;
typedef SmsSalePriceMap::iterator SmsSalePriceMapIter;

friend class Session;

public:
    DirectChargeThread(const char* name);
    ~DirectChargeThread();

    static bool start();

    bool init();

private:
    virtual void initWorkFuncMap();
    bool updateClientGivePool();
    bool initSmsSalePrice();

    void handleChargeReq(TMsg* pMsg);
    bool processChargeAgentOrAccount(Session* pSession);
    bool processChargeGivePool(Session* pSession);
    bool processChargeBalance(Session* pSession);

    void handleChargeDbRes(TMsg* pMsg);
    void handleDbUpdateReq(TMsg* pMsg);

    void getSalePrice(Session* pSession);
    bool responseToClient(Session* pSession);

private:
    SnManager m_snManager;

    SessionMap m_mapSession;

    AppDataMap m_mapAgentInfo;
    //AppDataVec m_vecUserPropertyLog;
	AppDataVecMap m_mapVecUserPropertyLog;

    AppDataVecMap m_mapVecGivePool;

    SmsSalePriceMap m_mapSmsSalePrice;

    vector<string> m_vecPrice;
};

extern DirectChargeThread* g_pDirectChargeThread;

#endif
