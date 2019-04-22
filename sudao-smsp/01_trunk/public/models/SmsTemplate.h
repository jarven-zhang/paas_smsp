#ifndef SmsTemplate_H_
#define SmsTemplate_H_
#include "platform.h"
#include <string>
#include <tr1/memory>


namespace models
{
	enum SMSTemplateBusType
	{
		SMS_TEMPLATE_WHITE = 0,
		SMS_TEMPLATE_BLACK = 1,
	};

    class SmsTemplate
    {
    
    public:
        SmsTemplate();
        virtual ~SmsTemplate();
        SmsTemplate(const SmsTemplate& other);
        SmsTemplate& operator=(const SmsTemplate& other);

    public:
        std::string m_strTemplateId;
        std::string m_strType;  //模板类型，0：通知模板，4：验证码模板，5：营销模板，7：USSD模板，8：闪信模板，9：挂机短信模板
        std::string m_strSmsType;   //模板短信类型，0：通知短信，4：验证码短信，5：营销短信，6：告警短信，7：USSD，8：闪信
        std::string m_strContent;  //内容
        std::string m_strSign;   //模板签名
        std::string m_strClientId;   //归属客户id
        Int32 m_iAgentId;  //归属代理商id
    };
} /* namespace models*/
#endif /* SmsTemplate_H_ */

