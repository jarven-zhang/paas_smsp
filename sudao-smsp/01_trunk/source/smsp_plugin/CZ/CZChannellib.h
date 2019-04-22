#ifndef CZChannellib_H_
#define CZChannellib_H_
#include "StateReport.h"
#include <vector>
#include <string>
#include "Channellib.h"

#define RSP_CODE_OK 						"200" //�ɹ�
#define RSP_CODE_NO_BUSINESS 				"210" //δ��ͨ����ҵ��
#define RSP_CODE_INSUFFICIENT_BALANCE 		"211" //����
#define RSP_CODE_HIGH_FREQUENCY 			"212" //����Ƶ�ʹ���
#define RSP_CODE_BLACKLIST 					"213" //�ֻ����뱻���������
#define RSP_CODE_OVERRATE 					"214" //24 Сʱ�ڣ��ֻ�����ķ��ʹ����Ѵ�����
#define RSP_CODE_SENSITIVED_WORD 			"215" //���д�
#define RSP_CODE_INVALID_MOBILE 			"216" //�ֻ����벻����
#define RSP_CODE_EXPIRED_MOBILE 			"217" //�û�ͣ��

#define REPORT_STATUS_CODE_OK 				"1" //�ɹ�

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
