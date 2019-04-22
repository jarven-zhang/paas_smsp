#ifndef MASCHANNELLIB_H_
#define MASCHANNELLIB_H_
#include "StateReport.h"
#include <vector>
#include <string>
#include "Channellib.h"

namespace smsp
{

    class MASChannellib:public Channellib
    {
    public:
        MASChannellib();
        virtual ~MASChannellib();
        void test();
        virtual bool parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason);
        virtual bool parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse);
        virtual int adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);		
    };

} /* namespace smsp */
#endif /* TXYCHANNELLIB_H_ */
