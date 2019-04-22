#ifndef TXYCHANNELLIB_H_
#define TXYCHANNELLIB_H_
#include "StateReport.h"
#include <vector>
#include <string>
#include "Channellib.h"
#include "StateResponse.h"

namespace smsp
{

    class TXYChannellib:public Channellib
    {
    public:
        TXYChannellib();
        virtual ~TXYChannellib();

        void test();
        virtual bool parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason);
        virtual bool parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse);
        virtual int  adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);		
		virtual bool parseSendMutiResponse(std::string respone, smsp::mapStateResponse &responses, std::string& strReason );	 
    };

} /* namespace smsp */
#endif /* TXYCHANNELLIB_H_ */
