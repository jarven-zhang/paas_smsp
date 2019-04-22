#ifndef CHChannellib_H_
#define CHChannellib_H_
#include "StateReport.h"
#include <vector>
#include <string>
#include "Channellib.h"

#define CH_FIELD_COMMAND				"command"
#define CH_FIELD_ACCOUNT				"spid"
#define CH_FIELD_MOBILES				"sa"
#define CH_FIELD_SMSID					"msgid"
#define CH_FIELD_MO_INFO				"moinfo"
#define CH_FIELD_MSG_STATUS				"mtstat"
#define CH_FIELD_STATUS					"status"
#define CH_FIELD_REPORT_STATUS			"rterrcode"

#define COMMAND_MT_REQ 					"MT_REQUEST"
#define COMMAND_MT_RSP 					"MT_RESPONSE"
#define COMMAND_RT_RSP 					"RT_RESPONSE"
#define COMMAND_MO_RSP 					"MO_RESPONSE"

namespace smsp {

class CHChannellib : public Channellib{
public:
	CHChannellib();
	virtual ~CHChannellib();
	virtual bool parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason);
	virtual bool parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes, std::string& strResponse);
	virtual int adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);
	int replace_all_distinct(string& str, const string& old_value, const string& new_value);
};

} /* namespace smsp */
#endif //// WTPARSER_H_ 
