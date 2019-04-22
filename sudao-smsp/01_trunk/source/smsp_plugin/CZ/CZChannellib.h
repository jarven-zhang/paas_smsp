#ifndef CZChannellib_H_
#define CZChannellib_H_
#include "StateReport.h"
#include <vector>
#include <string>
#include "Channellib.h"

#define RSP_CODE_OK 						"200" //成功
#define RSP_CODE_NO_BUSINESS 				"210" //未开通此项业务
#define RSP_CODE_INSUFFICIENT_BALANCE 		"211" //余额不足
#define RSP_CODE_HIGH_FREQUENCY 			"212" //发送频率过高
#define RSP_CODE_BLACKLIST 					"213" //手机号码被列入黑名单
#define RSP_CODE_OVERRATE 					"214" //24 小时内，手机号码的发送次数已达上限
#define RSP_CODE_SENSITIVED_WORD 			"215" //敏感词
#define RSP_CODE_INVALID_MOBILE 			"216" //手机号码不存在
#define RSP_CODE_EXPIRED_MOBILE 			"217" //用户停机

#define REPORT_STATUS_CODE_OK 				"1" //成功

#define MAX_PARAMS_COUNT					10

namespace smsp {

class CZChannellib : public Channellib{
public:
	CZChannellib();
	virtual ~CZChannellib();
	virtual bool parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason);
	virtual bool parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes, std::string& strResponse);
	virtual int adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);
	
};

} /* namespace smsp */
#endif //// WTPARSER_H_ 
