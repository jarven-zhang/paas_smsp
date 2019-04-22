#include "BlackList.h"
#include "stdlib.h"

namespace models
{
	Int16 Blacklist::GetSmsTypeFlag(string& strSmsType)
	{
		int iSmsType = atoi(strSmsType.c_str());
		switch(iSmsType)
		{
			case SMS_TYPE_NOTICE: 
				return SMS_TYPE_NOTICE_FLAG;
				
			case SMS_TYPE_VERIFICATION_CODE: 
				return SMS_TYPE_VERIFICATION_CODE_FLAG;
				
			case SMS_TYPE_MARKETING: 
				return SMS_TYPE_MARKETING_FLAG;
				
			case SMS_TYPE_ALARM: 
				return SMS_TYPE_ALARM_FLAG;
				
			case SMS_TYPE_USSD: 
				return SMS_TYPE_USSD_FLAG;
				
			case SMS_TYPE_FLUSH_SMS: 
				return SMS_TYPE_FLUSH_SMS_FLAG;
				
			default: 
				return 0;
		}	
		return 0;
	}

}
/* namespace models */
/* namespace models */
















