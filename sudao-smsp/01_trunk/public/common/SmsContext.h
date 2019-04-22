#ifndef SMSCONTEXT_H_
#define SMSCONTEXT_H_
#include "platform.h"
#include <string>
#include <tr1/memory>
#include "Uuid.h"
#include "Channel.h"

namespace smsp
{
    class SmsContext
    {
    public:
        SmsContext();
        SmsContext(const SmsContext& other);
        SmsContext& operator=(const SmsContext& other);
        void release()
        {
            delete this;
        }
		////insert database use
		string m_strSmsUuid;    
		Int32 m_iChannelId;     /* ͨ��ID */
        string m_strChannelLibName;
		UInt32 m_uChannelOperatorsType; /*ͨ����Ӫ����*/
		string m_strChannelRemark; 
		Int32 m_iOperatorstype; /*������Ӫ��*/
		Int32 m_iArea; /*����*/
		string m_strSmsId; /*����smsid*/
		string m_strUserName;
		string m_strContent; //����
        Int32 m_iSmscnt;	 //�۷�����
        Int32 m_iStatus;     // ״̬
		string m_strPhone;   //�ֻ�����
		string m_strRemark; 
		Int32 m_iSmsfrom;    //
		Int32 m_iIndexNum;   //��Ϣ���
		float m_fCostFee;
		string m_strChannelSmsId;
		UInt32 m_uPayType;
		string m_strClientId;
		string m_strErrorCode;
		Int64 m_lSendDate;
		string m_strSubmit;
		Int64 m_lSubmitDate;
		string m_strSubret;
		Int64 m_lSubretDate;
		string m_strReport;
		Int64 m_lReportDate;
		Int64 m_lRecvReportDate;
		string m_strShowPhone;
		UInt64 m_uSendId;
		UInt32 m_ulongsms; ////0 shot sms or 1 long sms
		UInt32 m_uClientCnt;
		UInt32 m_uChannelCnt;
		string m_strC2sTime;

		////response use
		string m_strChannelname;
		string m_strYZXErrCode;  //added by weilu for tou chuan report
		UInt32 m_uChannelIdentify;

		////report use
        UInt32 m_uDivideNum;
		UInt64 m_uC2sId;
		
		//ģ������������
        std::string m_strTemplateId;
        std::string m_strTemplateParam;
        std::string m_strChannelTemplateId;
        std::string m_strAppSecret;
        std::string m_strAppKey;
        std::string m_strAccessToken;

		/////yunmas channel add 
		string m_strMasUserId;
		string m_strMasAccessToken;
		string m_strDisplaynum;
		UInt64 m_uBelong_sale;
		UInt64 m_uBelong_business;
		string m_strSign; /*ǩ��*/

		UInt32 m_uSmsType; /* ��������*/

		UInt32 m_uShowSignType; /*�Ժ�����*/
		string m_strAccessids; /*����ID*/
		string m_strSignPort;
		UInt32 m_uHttpmode;
		bool   m_bResendCfgFlag;
		UInt32 m_uResendTimes;//failed resend times ,get form mq msg
		string m_strFailedChannel;//failed resend channel,get form mq msg
		string m_strShowSignType_c2s;
		string m_strUserExtendPort_c2s;
		string m_strSignExtendPort_c2s;
		string m_strChannelOverrate_access;
		Int32  m_oriChannelId;
		models::Channel m_Channel;
    };
    typedef std::tr1::shared_ptr<SmsContext> SmsContextPtr;

} /* namespace smsp */
#endif /* SMSCONTEXT_H_ */
