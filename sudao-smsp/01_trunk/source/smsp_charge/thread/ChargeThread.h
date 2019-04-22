#ifndef _CHARGE_Thread_h_
#define _CHARGE_Thread_h_
#include <set>
#include <string.h>
#include "Thread.h"
#include "SnManager.h"
#include "SmspUtils.h"
#include "SignExtnoGw.h"
#include "CustomerOrder.h"
#include "CloudCustomerOrder.h"
#include "CloudCustomerPurchase.h"
#include "OEMClientPool.h"
#include "CDBThread.h"

using std::map;
using std::set;

using namespace models;

typedef list<CustomerOrder> CustomerOrderLIST;
typedef list<CloudCustomerOrder> CloudCustomerOrderLIST;
typedef list<OEMClientPool> OEMClientPoolLIST;

#define OEM_SESSION_TIMER_STRING            "OemSessionTimerString"
#define BRAND_OR_SALE_SESSION_TIMER_STRING  "BrandOrSaleSessionTimerString"
#define CLOUD_SESSION_TIMER_STRING          "CloudSessionTimerString"

class THttpQueryReq: public TMsg
{
public:
    THttpQueryReq()
    {
        m_uAgentType = 0;
        m_uOperater = 0;
        m_uArea = 0;
        m_uCount = 0;
        m_uClientAscription = 0;
    }

    string m_strClientID;
    string m_strProductType;  //多个产品类型之间以英文逗号间隔
    UInt32 m_uAgentType;    //1:销售代理商,2:品牌代理商,3:资源合作商,4:代理商和资源合作,5:OEM代理商

    UInt32 m_uOperater; //运营商，0全网 1移动 2联通 3电信 4国际
    UInt32 m_uArea;     //对应t_sms_area表中一级area_id(省)

    UInt32 m_uCount;
    string m_strFee;

    string m_strSmsID;
    string m_strPhone;
    UInt32 m_uClientAscription; // 0 = a li,1 = dai li shang ,2 = yun ping tai

};

class ChgSession
{
public:
    ChgSession()
    {
    }

    UInt64 m_lchargetime;
    string m_strClientID;
    string m_uProductType;
    UInt32 m_uAgentType;
    UInt32 m_uCount;
    string m_strFee;
    UInt64 m_strSubID;
    UInt64 m_uSeqRet;       //useqId for response response

    string m_strSmsID;
    string m_strPhone;

    double m_fUintPrice;
    double m_fSalePrice;

    double m_fProductCost;
    double m_fQuantity;

    THttpQueryReq m_ChargeReq;

    CThreadWheelTimer* m_pTimer;
};


class OEMChgSession
{
public:
    OEMChgSession()
    {
    }

    UInt64 m_lchargetime;
    string m_strClientID;
    string m_uProductType;
    UInt32 m_uAgentType;
    UInt32 m_uCount;
    string m_strFee;
    string m_strSubID;

    UInt64 m_uSeqRet;       //useqId for response response
    string m_strSmsID;
    string m_strPhone;
    double m_fUintPrice;            //OEM 订单单价

    THttpQueryReq m_ChargeReq;

    CThreadWheelTimer* m_pTimer;
};

class CloudChargeSession
{
public:
    CloudChargeSession()
    {
        m_lchargetime = 0;
        m_uProductType = 0;
        m_uCount = 0;
        //m_uOrderID = 0;
        m_uSeqRet = 0;
        m_fUnitPrice = 0;
        //m_uPurchaseId = 0;
        m_uAgentType = 0;
    }
    UInt64 m_lchargetime;
    string m_strClientID;
    UInt32 m_uProductType;
    UInt32 m_uCount;
    string m_strFee;        ///guo ji daun xin fei lu
    string m_uOrderID;
    //UInt64 m_uProductID;
    UInt64 m_uSeqRet;       //useqId for response response
    float m_fUnitPrice; ///pu tong daun xin dan jia
    string m_strSmsID;
    string m_strPhone;
    string m_uPurchaseId;
    UInt32 m_uAgentType;
    CThreadWheelTimer* m_pTimer;
};

class ChargeThread: public CThread
{
public:
    ChargeThread(const char* name);
    ~ChargeThread();

    static bool start();

    bool Init();
    UInt32 GetSessionMapSize() {return m_SesssionMap.size();}

private:

    void initWorkFuncMap();

    void handleTimeoutReqMsg(TMsg* pMsg);
    void handleChargeReqMsg(TMsg* pMsg);
    void handleDbRspMsg(TMsg* pMsg);
    void handleDbUpdateReqMsg(TMsg* pMsg);


    UInt32 HttpResponse(UInt32 iReturn, UInt64 uSeqRet, string strSmsID, string strPhone, string strSubID, double fSlaePrice, double uProductCost, UInt32 uProductType = 99);

    void BrandOrSalesDBResp(TDBQueryResp* pResp);
    void OemDBResp(TDBQueryResp* pResp);

    void OemCharging(THttpQueryReq* pReq, UInt64 uSeq, UInt64 uLastChargeFailClientPoolId = 0);
    void BrandOrSalesCharging(THttpQueryReq* pReq, UInt64 uSeq, UInt64 uLastChargeFailSubId = 0);

    void BrandOrSaleClearHttpSession(UInt64 uKey);
    void OemClearHttpSession(UInt64 uKey);

private:
    SnManager m_SnMng;
    SmspUtils m_SmspUtils;

    map<string, CustomerOrderLIST> m_CustomerOrderMap;             //key client_producttype
    set<UInt64> m_CustomerOrderSetKey;

    map<UInt64, ChgSession> m_SesssionMap;

    map<string, OEMClientPoolLIST> m_OEMClientPoolMap;
    set<UInt64> m_OEMClientPoolSetKey;

    map<UInt64, OEMChgSession> m_OEMSesssionMap;
};

extern ChargeThread* g_pChargeThread;

#endif
