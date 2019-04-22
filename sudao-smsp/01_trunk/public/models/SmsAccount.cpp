#include "SmsAccount.h"


SmsAccount::SmsAccount()
{
	m_strAccount = "";
	m_strPWD = "";
	m_strname = "";
	////m_fFee = 0;
	m_strSid = "";
	m_uStatus = 0;			
	m_uNeedReport = 0; 		
	m_uNeedMo = 0; 			
	m_uNeedaudit = 0; 		
	m_strCreattime = ""; 
	m_strIP = "";		
	m_strMoUrl = "";	
	m_uNodeNum = 0;
	m_uPayType = 0;			
	////m_uOverrate_num = 0;
	////m_strCsign = "";
	////m_strEsign = "";
	////m_strCompany = "";
	m_uLoginErrorCount = 0;
	m_uLockTime = 0;
	m_bAllIpAllow = false;
	m_uLinkCount = 0;
	////m_uNeedSign = 0;
    m_uNeedExtend = 0;
    m_uNeedSignExtend = 0;
    ////m_fGjFee = 0;
    m_strDeliveryUrl = "";
    m_uAgentId = 0;
    m_uSmsFrom = 0;
    m_uOverRateCharge = 0;
	loginMode = 0;
	m_uGetRptInterval = 5;	//5S
	m_uGetRptMaxSize = 30;	//

    m_uSgipRtMoPort = 0;
    m_uSgipNodeId = 0;
	m_uIdentify = 0;

	m_uClientLeavel = 0;
	m_uClientAscription = 0;
	m_uSignPortLen = 0;
	m_uBelong_sale = 0;
    m_ucHttpProtocolType = INVALID_UINT8;
    m_iPoolPolicyId = -1;
	m_uSupportLongMo = 0;
    m_uFailedResendFlag = 0;
	m_uAccountExtAttr = 0x00;
    m_uGroupsendLimCfg = 0;
    m_uGroupsendLimUserflag = 0;
    m_uGroupsendLimNum = 0;
    m_uGroupsendLimTime = 0;

    m_uiSpeed = INVALID_UINT32;

}

SmsAccount::~SmsAccount()
{
}

