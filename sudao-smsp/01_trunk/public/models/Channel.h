
#ifndef CHANNEL_H_
#define CHANNEL_H_
#include <string>
#include <iostream>
#include "platform.h"

using namespace std;

enum ChannelHttpMode
{
    HTTP_GET  = 1,
    HTTP_POST = 2,
    YD_CMPP   = 3,
    DX_SMGP   = 4,
    LT_SGIP   = 5,
    GJ_SMPP   = 6,
    YD_CMPP3  = 7,
};


enum ExValue
{
    CHANNEL_CHINESE_SUPORT_FALG = 0x1,//means this channel suport chinese
    CHANNEL_ENGLISH_SUPORT_FALG = 0x2,//means this channel suport english
    CHANNEL_REPORT_CACHE_FALG   = 0x4,//mesns save report to mq
    CHANNEL_PRIORITY_FLAG       = 0x8,   //means open channel's priority
    CHANNEL_OWN_SPLIT_FLAG      = 0x10,
};

enum ChannelCmppType
{
    CMPP_COUNTRY = 0,
    CMPP_FUJIAN = 1,
};

class FailedResendSysCfg
{
public:
    FailedResendSysCfg()
    {
        m_uResendMaxTimes = 0;
        m_uResendMaxDuration = 0;
    };
    UInt32 m_uResendMaxTimes;
    UInt32 m_uResendMaxDuration;// s
};

namespace models
{

    class Channel
    {
    public:
        Channel();
        virtual ~Channel();
        Channel(const Channel &other)
        {
            this->_showsigntype = other._showsigntype;
            this->_accessID = other._accessID;
            this->_clientID = other._clientID;
            this->_password = other._password;
            this->_channelRule = other._channelRule;
            this->channelID = other.channelID;
            this->chanenlNum = other.chanenlNum;
            this->channelname = other.channelname;
            this->area = other.area;
            this->operatorstyle = other.operatorstyle;
            this->industrytype = other.industrytype;
            this->longsms = other.longsms;
            this->wappush = other.wappush;
            this->httpmode = other.httpmode;
            this->url = other.url;
            this->coding = other.coding;
            this->postdata = other.postdata;
            this->querystatepostdata = other.querystatepostdata;
            this->querystateurl = other.querystateurl;
            this->costprice = other.costprice;
            this->balance = other.balance;
            this->expenditure = other.expenditure;
            this->shownum = other.shownum;
            this->nodeID = other.nodeID;
            this->spID = other.spID;
            this->lastWarnTime = other.lastWarnTime;
            this->m_strQueryUpPostData = other.m_strQueryUpPostData;
            this->m_strQueryUpUrl = other.m_strQueryUpUrl;
            this->m_strServerId = other.m_strServerId;
            this->m_uSendNodeId = other.m_uSendNodeId;
            this->m_uChannelType = other.m_uChannelType;
            this->m_uWhiteList = other.m_uWhiteList;
            this->m_uAccessSpeed = other.m_uAccessSpeed;
            this->m_uSpeed = other.m_uSpeed;
            this->m_uExtendSize = other.m_uExtendSize;
            this->m_uNeedPrefix = other.m_uNeedPrefix;
            this->m_strRemark = other.m_strRemark;
            this->m_strBeginTime = other.m_strBeginTime;
            this->m_strEndTime = other.m_strEndTime;
            this->m_uChannelIdentify = other.m_uChannelIdentify;
            this->m_uSliderWindow = other.m_uSliderWindow;
            this->m_uChannelMsgQueueMaxSize = other.m_uChannelMsgQueueMaxSize;
            this->m_uMqId = other.m_uMqId;
            this->m_strOauthUrl = other.m_strOauthUrl;
            this->m_strOauthData = other.m_strOauthData;
            this->m_uContentLength = other.m_uContentLength;
            this->m_uSegcodeType = other.m_uSegcodeType;
            this->m_uBelong_business = other.m_uBelong_business;
            this->m_uClusterType = other.m_uClusterType;
            this->m_uclusterMaxNumber = other.m_uclusterMaxNumber;
            this->m_uClusterMaxTime = other.m_uClusterMaxTime;
            this->m_uProtocolType = other.m_uProtocolType;
            this->m_uExValue = other.m_uExValue;
            this->m_strChannelLibName = other.m_strChannelLibName;
            this->m_uLinkNodenum = other.m_uLinkNodenum;
            this->m_strMoPrefix       = other.m_strMoPrefix;
            this->m_strResendErrCode = other.m_strResendErrCode;
            this->m_strRespClientId = other.m_strRespClientId;
            this->m_strRespPasswd = other.m_strRespPasswd;
            this->m_strExStr = other.m_strExStr;
            this->m_strSgipReportUrl = other.m_strSgipReportUrl;

            this->m_uSplitRule = other.m_uSplitRule;
            this->m_uCnLen = other.m_uCnLen;
            this->m_uEnLen = other.m_uEnLen;
            this->m_uCnSplitLen = other.m_uCnSplitLen;
            this->m_uEnSplitLen = other.m_uEnSplitLen;

        }
        Channel &operator =(const Channel &other);
        bool operator <(const Channel &rhs) const // 升序排序时必须写的函数
        {
            return channelID < rhs.channelID;
        }

    public:
        string _accessID;
        string _clientID;
        string _password;
        string _channelRule; ///showsign
        Int32 channelID;
        Int32 chanenlNum;
        string channelname;
        UInt32 area;
        UInt32 operatorstyle;
        UInt32 industrytype;
        UInt32 longsms;
        UInt32 wappush;
        UInt32 httpmode;        //1: GET   2:POST   3:CMPP ....
        UInt32 _showsigntype;
        string url;
        string coding;
        string postdata;
        double balance;
        double expenditure;
        double costprice;
        string querystateurl;
        string querystatepostdata;
        UInt32 shownum;
        std::string spID;
        UInt32 nodeID;
        UInt64 lastWarnTime;
        std::string m_strQueryUpUrl;
        std::string m_strQueryUpPostData;
        std::string m_strServerId;
        UInt64 m_uSendNodeId;
        UInt32 m_uChannelType;
        UInt32 m_uWhiteList;
        UInt32 m_uSpeed;
        UInt32 m_uAccessSpeed;
        UInt32 m_uExtendSize;
        UInt32 m_uNeedPrefix;   ///// 0 is not add 86, 1 is add 86
        string m_strRemark;
        string m_strBeginTime;  //blank: channel always available. else eg 08:00
        string m_strEndTime;    //blank: channel always available. else eg 08:00
        UInt32 m_uChannelIdentify;      //index of t_sms__record_*

        string m_strOauthUrl;
        string m_strOauthData;
        UInt32 m_uContentLength;
        int m_uSliderWindow;
        int m_uChannelMsgQueueMaxSize;

        UInt64 m_uMqId;
        UInt32 m_uSegcodeType;

        UInt64 m_uBelong_business; /* 归属商务*/

        UInt32 m_uClusterType;      /*为1时候组包*/
        UInt32 m_uclusterMaxNumber; /*最大组包个数*/
        UInt32 m_uClusterMaxTime;   /*最大组包时间*/
        UInt32 m_uProtocolType;/*协议子类型，针对cmpp协议 1:福建省网cmpp*/

        UInt32 m_uExValue;      // 扩展属性，1：中文，2：英文，4：缓存状态报告到MQ队列,8: 开启通道队列优先级，16：启用自定义拆分规则

        std::string m_strChannelLibName;
        UInt32 m_uLinkNodenum;      /*直连协议连接数*/
        std::string m_strMoPrefix;
        std::string m_strResendErrCode; //channel report those error code will resend sms

        std::string m_strRespClientId;
        std::string m_strRespPasswd;
        std::string m_strExStr;
        std::string m_strSgipReportUrl;//when accout is 0 and password is 0 for sgip report link

        UInt32 m_uSplitRule;
        UInt32 m_uCnLen;
        UInt32 m_uEnLen;
        UInt32 m_uCnSplitLen;
        UInt32 m_uEnSplitLen;
    };

} /* namespace models */
#endif /* CHANNEL_H_ */
