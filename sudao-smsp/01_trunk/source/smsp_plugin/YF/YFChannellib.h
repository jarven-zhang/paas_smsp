/*
YFÍøÂç
*/
#ifndef YFCHANNELLIB_H_
#define YFCHANNELLIB_H_

#include "StateReport.h"
#include <vector>
#include <string>
#include "Channellib.h"

namespace smsp
{

    class YFChannellib:public Channellib
    {
    public:
        YFChannellib();
        virtual ~YFChannellib();

        void test();
        virtual bool parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason);
        virtual bool parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes, std::string& strResponse);
        virtual int adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);

    };

} /* namespace smsp */
#endif /* YFPARSER_H_ */