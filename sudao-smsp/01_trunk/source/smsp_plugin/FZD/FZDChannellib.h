#ifndef FZDCHANNELLIB_H_
#define FZDCHANNELLIB_H_

#include "StateReport.h"
#include <vector>
#include <string>
#include "Channellib.h"
//#include "UrlCode.h"

namespace smsp
{

    class FZDChannellib:public Channellib
    {
    public:
        FZDChannellib();
        virtual ~FZDChannellib();

        void test();
        virtual bool parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason);
        virtual bool parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse);
        virtual int adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);

    };

} /* namespace smsp */
#endif /* FZDCHANNELLIB_H_ */