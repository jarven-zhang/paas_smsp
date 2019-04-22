#ifndef ZSSDHTTPChannellib_H_
#define ZSSDHTTPChannellib_H_

#include "StateReport.h"
#include <vector>
#include <string>
#include "Channellib.h"

namespace smsp
{

    class ZSSDHTTPChannellib:public Channellib
    {
    public:
        ZSSDHTTPChannellib();
        virtual ~ZSSDHTTPChannellib();

        void test();
        virtual bool parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason);
        virtual bool parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse);
        virtual int adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);
    };

} /* namespace smsp */
#endif /* ZSSDHTTPChannellib_H_ */