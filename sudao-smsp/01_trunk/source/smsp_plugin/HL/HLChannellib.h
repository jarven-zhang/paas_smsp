#ifndef HLChannellib_H_
#define HLChannellib_H_
#include "StateReport.h"
#include <vector>
#include <string>
#include "Channellib.h"

//#define FIELD_COMMAND				"command"
#define FIELD_ACCOUNT				"user"
#define FIELD_MOBILES				"phone"
#define FIELD_SMSID					"taskid"
#define FIELD_MSG					"message"
//#define FIELD_MSG_STATUS				"mtstat"
#define FIELD_STATUS				"mr"
//#define FIELD_REPORT_STATUS			"rterrcode"

#define MT_RSP_CODE_OK 					"OK"
#define MT_RSP_CODE_ERR 				"ERR"

#define RT_RSP_CODE_OK_1 				"DELIVRD"
#define RT_RSP_CODE_OK_2 				"0"

namespace smsp {

class HLChannellib : public Channellib{
public:
	HLChannellib();
	virtual ~HLChannellib();
	virtual bool parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason);
	virtual bool parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes, std::string& strResponse);
	virtual int adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);
	
};

} /* namespace smsp */
#endif //// WTPARSER_H_ 
