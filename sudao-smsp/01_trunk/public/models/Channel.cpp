
#include "Channel.h"

namespace models
{

    Channel::Channel()
    {
        // TODO Auto-generated constructor stub
        channelID=0;
        area=0;
        operatorstyle=0;
        industrytype=0;
        longsms=0;
        wappush=0;
        httpmode=0;
        balance=0.0;
        expenditure=0.0;
        costprice=0.0;
        shownum=0;
        chanenlNum=0;
        nodeID=0;
        _showsigntype=0;
        lastWarnTime=0;
        channelname = "";
        m_strQueryUpPostData = "";
        m_strQueryUpUrl = "";
        m_strServerId = "";
        m_uSendNodeId = 0;
        m_uChannelType = 0;
        m_uWhiteList =0;
        m_uAccessSpeed = 0;
        m_uSpeed = 0;
        m_uExtendSize = 0;
        m_uNeedPrefix = 0;
        m_uChannelIdentify =0;
        m_uSliderWindow = 0;
        m_uChannelMsgQueueMaxSize = 0;
        m_uMqId = 0;
        m_uContentLength = 0;
        m_uBelong_business = 0;
        m_uClusterType = 0;    
        m_uclusterMaxNumber = 0; 
        m_uClusterMaxTime = 0;   
        m_uProtocolType = 0;
        m_uExValue = CHANNEL_CHINESE_SUPORT_FALG|CHANNEL_ENGLISH_SUPORT_FALG;//Ä¬ÈÏÖ§³ÖÖÐÓ¢ÎÄ
        m_strResendErrCode = "";
        m_uSplitRule = 0;
        m_uCnLen = 70;
        m_uEnLen = 140; //æ ‡å‡†åè®®ï¼š140 éžæ ‡å‡†åè®®ï¼š70
        m_uCnSplitLen = 67;
        m_uEnSplitLen = 67;
    }

    Channel::~Channel()
    {
        // TODO Auto-generated destructor stub
    }
    Channel& Channel::operator =(const Channel& other)
    {
        this->_showsigntype=other._showsigntype;
        this->_accessID=other._accessID;
        this->_clientID=other._clientID;
        this->_password=other._password;
        this->_channelRule=other._channelRule;
        this->channelID=other.channelID;
        this->chanenlNum=other.chanenlNum;
        this->channelname=other.channelname;
        this->area=other.area;
        this->operatorstyle=other.operatorstyle;
        this->industrytype=other.industrytype;
        this->longsms=other.longsms;
        this->wappush=other.wappush;
        this->httpmode=other.httpmode;
        this->url=other.url;
        this->coding=other.coding;
        this->postdata=other.postdata;
        this->querystatepostdata=other.querystatepostdata;
        this->querystateurl=other.querystateurl;
        this->costprice=other.costprice;
        this->balance=other.balance;
        this->expenditure=other.expenditure;
        this->shownum=other.shownum;
        this->nodeID=other.nodeID;
        this->spID=other.spID;
        this->lastWarnTime=other.lastWarnTime;
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
        return *this;
    }
} /* namespace models */
