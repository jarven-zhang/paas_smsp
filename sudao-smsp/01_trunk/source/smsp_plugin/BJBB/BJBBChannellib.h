#ifndef BJBBCHANNELLIB_H_
#define BJBBCHANNELLIB_H_
#include "StateReport.h"
#include <vector>
#include <map>
#include <string>
#include "Channellib.h"

namespace smsp
{

    class BJBBChannellib:public Channellib
    {
    public:
        BJBBChannellib();
        virtual ~BJBBChannellib();

        void test();
        virtual bool parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason);
        virtual bool parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse);
        virtual int adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);
    };

} /* namespace smsp */
#endif /* BJBBPARSER_H_ */