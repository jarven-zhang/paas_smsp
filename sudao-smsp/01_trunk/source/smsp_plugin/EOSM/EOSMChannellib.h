#ifndef EOSMCHANNELLIB_H_
#define EOSMCHANNELLIB_H_
#include "StateReport.h"
#include <vector>
#include <string>
#include "Channellib.h"

namespace smsp
{
    class EOSMChannellib:public Channellib
    {
    public:
        EOSMChannellib();
        virtual ~EOSMChannellib();

        void test();
        virtual bool parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason);
        virtual bool parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse);
        virtual int adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);
        virtual int getChannelInterval(SmsChlInterval& smsChIntvl);
    };

} /* namespace smsp */
#endif /* GZLDPARSER_H_ */