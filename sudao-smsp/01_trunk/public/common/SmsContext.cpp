#include "SmsContext.h"

namespace smsp
{
    SmsContext::SmsContext()
    {

		m_strSmsUuid = getUUID();
		m_iChannelId = 0;
		m_uChannelOperatorsType = 0;
		m_iOperatorstype = 0; 
		m_iArea = 0;
        m_iSmscnt = 0;
        m_iStatus = 0; 
		m_iSmsfrom = 0;
		m_iIndexNum = 0;
		m_fCostFee = 0;
		m_uPayType = 0;
		m_lSendDate = 0;
		m_lSubmitDate = 0;
		m_lSubretDate = 0;
		m_lReportDate = 0;
		m_lRecvReportDate = 0;
		m_uSendId = 0;
		m_ulongsms = 0;
		m_uClientCnt = 0;
		m_uChannelCnt = 0;
		m_uChannelIdentify = 0;
		
        m_uDivideNum = 0;
		m_uC2sId = 0;
		m_uBelong_sale = 0;
		m_uBelong_business = 0;
		m_uSmsType = 0;
		m_uHttpmode = 0;
		m_bResendCfgFlag = false;
		m_uResendTimes = 0;
		m_strFailedChannel = "";
		m_strShowSignType_c2s = "";
		m_strUserExtendPort_c2s = "";
		m_strSignExtendPort_c2s = "";

		m_oriChannelId = 0;
    }

    SmsContext::SmsContext(const SmsContext& other)
    {
		this->m_strSmsUuid = other.m_strSmsUuid;
		this->m_iChannelId = other.m_iChannelId;
		this->m_uChannelOperatorsType = other.m_uChannelOperatorsType;
		this->m_strChannelRemark = other.m_strChannelRemark;
		this->m_iOperatorstype = other.m_iOperatorstype; 
		this->m_iArea = other.m_iArea;
		this->m_strSmsId = other.m_strSmsId;
		this->m_strUserName = other.m_strUserName;
		this->m_strContent = other.m_strContent;
		this->m_iSmscnt = other.m_iSmscnt;
		this->m_iStatus = other.m_iStatus; 
		this->m_strPhone = other.m_strPhone;
		this->m_strRemark = other.m_strRemark;
		this->m_iSmsfrom = other.m_iSmsfrom;
		this->m_iIndexNum = other.m_iIndexNum;
		this->m_fCostFee = other.m_fCostFee;
		this->m_strChannelSmsId = other.m_strChannelSmsId;
		this->m_uPayType = other.m_uPayType;
		this->m_strClientId = other.m_strClientId;
		this->m_strErrorCode = other.m_strErrorCode;
		this->m_lSendDate = other.m_lSendDate;
		this->m_strSubmit = other.m_strSubmit;
		this->m_lSubmitDate = other.m_lSubmitDate;
		this->m_strSubret = other.m_strSubret;
		this->m_lSubretDate = other.m_lSubretDate;
		this->m_strReport = other.m_strReport;
		this->m_lReportDate = other.m_lReportDate;
		this->m_lRecvReportDate = other.m_lRecvReportDate;
		this->m_strShowPhone = other.m_strShowPhone;
		this->m_uSendId = other.m_uSendId;
		this->m_ulongsms = other.m_ulongsms;
		this->m_uClientCnt = other.m_uClientCnt;
		this->m_uChannelCnt = other.m_uChannelCnt;
		this->m_strC2sTime = other.m_strC2sTime;

		////response use
		this->m_strChannelname = other.m_strChannelname;
		this->m_strYZXErrCode = other.m_strYZXErrCode;
		this->m_uChannelIdentify = other.m_uChannelIdentify;

		this->m_uDivideNum = other.m_uDivideNum;
		this->m_uC2sId = other.m_uC2sId;
		
		m_strTemplateParam = other.m_strTemplateParam;
		m_strChannelTemplateId = other.m_strChannelTemplateId;
		m_strAppSecret = other.m_strAppSecret;
		m_strAppKey = other.m_strAppKey;
		m_strAccessToken = other.m_strAccessToken;

		m_strMasAccessToken = other.m_strMasAccessToken;
		m_strMasUserId = other.m_strMasUserId;
		m_uBelong_sale = other.m_uBelong_sale;		
		m_uBelong_business = other.m_uBelong_business;		
		m_strSign  = other.m_strSign;
		m_uSmsType  = other.m_uSmsType;
		m_uShowSignType =  other.m_uShowSignType;
		m_strAccessids = other.m_strAccessids; 
		m_strChannelLibName = other.m_strChannelLibName;
		m_strSignPort = other.m_strSignPort;
		this->m_uHttpmode = other.m_uHttpmode;
		this->m_bResendCfgFlag = other.m_bResendCfgFlag;
		this->m_uResendTimes = other.m_uResendTimes;
		this->m_strFailedChannel = other.m_strFailedChannel;
		this->m_strShowSignType_c2s = other.m_strShowSignType_c2s;
		this->m_strUserExtendPort_c2s = other.m_strUserExtendPort_c2s;
		this->m_strSignExtendPort_c2s = other.m_strSignExtendPort_c2s;
		this->m_strChannelOverrate_access = other.m_strChannelOverrate_access;
		this->m_oriChannelId = other.m_oriChannelId;
		this->m_Channel = other.m_Channel;
    }

    SmsContext& SmsContext::operator =(const SmsContext& other)
    {
		if (this == &other)
		{
			return *this;
		}

		this->m_strSmsUuid = other.m_strSmsUuid;
		this->m_iChannelId = other.m_iChannelId;
		this->m_uChannelOperatorsType = other.m_uChannelOperatorsType;
		this->m_strChannelRemark = other.m_strChannelRemark;
		this->m_iOperatorstype = other.m_iOperatorstype; 
		this->m_iArea = other.m_iArea;
		this->m_strSmsId = other.m_strSmsId;
		this->m_strUserName = other.m_strUserName;
		this->m_strContent = other.m_strContent;
		this->m_iSmscnt = other.m_iSmscnt;
		this->m_iStatus = other.m_iStatus; 
		this->m_strPhone = other.m_strPhone;
		this->m_strRemark = other.m_strRemark;
		this->m_iSmsfrom = other.m_iSmsfrom;
		this->m_iIndexNum = other.m_iIndexNum;
		this->m_fCostFee = other.m_fCostFee;
		this->m_strChannelSmsId = other.m_strChannelSmsId;
		this->m_uPayType = other.m_uPayType;
		this->m_strClientId = other.m_strClientId;
		this->m_strErrorCode = other.m_strErrorCode;
		this->m_lSendDate = other.m_lSendDate;
		this->m_strSubmit = other.m_strSubmit;
		this->m_lSubmitDate = other.m_lSubmitDate;
		this->m_strSubret = other.m_strSubret;
		this->m_lSubretDate = other.m_lSubretDate;
		this->m_strReport = other.m_strReport;
		this->m_lReportDate = other.m_lReportDate;
		this->m_lRecvReportDate = other.m_lRecvReportDate;
		this->m_strShowPhone = other.m_strShowPhone;
		this->m_uSendId = other.m_uSendId;
		this->m_ulongsms = other.m_ulongsms;
		this->m_uClientCnt = other.m_uClientCnt;
		this->m_uChannelCnt = other.m_uChannelCnt;
		this->m_strC2sTime = other.m_strC2sTime;

		////response use
		this->m_strChannelname = other.m_strChannelname;
		this->m_strYZXErrCode = other.m_strYZXErrCode;
		this->m_uChannelIdentify = other.m_uChannelIdentify;

		this->m_uDivideNum = other.m_uDivideNum;
		this->m_uC2sId = other.m_uC2sId;
		
		m_strTemplateParam = other.m_strTemplateParam;
		m_strChannelTemplateId = other.m_strChannelTemplateId;
		m_strAppSecret = other.m_strAppSecret;
		m_strAppKey = other.m_strAppKey;
		m_strAccessToken = other.m_strAccessToken;
		m_strMasAccessToken = other.m_strMasAccessToken;
		m_strMasUserId = other.m_strMasUserId;
		m_uBelong_sale = other.m_uBelong_sale;		
		m_uBelong_business = other.m_uBelong_business;		
		m_strSign  = other.m_strSign;
		m_uSmsType  = other.m_uSmsType;
		m_uShowSignType =  other.m_uShowSignType;
		m_strAccessids = other.m_strAccessids; 
		m_strChannelLibName = other.m_strChannelLibName;
		m_strSignPort = other.m_strSignPort;
		this->m_uHttpmode = other.m_uHttpmode;
		this->m_bResendCfgFlag = other.m_bResendCfgFlag;
		this->m_uResendTimes = other.m_uResendTimes;
		this->m_strFailedChannel = other.m_strFailedChannel;
		this->m_strShowSignType_c2s = other.m_strShowSignType_c2s;
		this->m_strUserExtendPort_c2s = other.m_strUserExtendPort_c2s;
		this->m_strSignExtendPort_c2s = other.m_strSignExtendPort_c2s;
		this->m_strChannelOverrate_access = other.m_strChannelOverrate_access;
		this->m_oriChannelId = other.m_oriChannelId;
		this->m_Channel = other.m_Channel;
        return *this;
    }

} /* namespace smsp */

