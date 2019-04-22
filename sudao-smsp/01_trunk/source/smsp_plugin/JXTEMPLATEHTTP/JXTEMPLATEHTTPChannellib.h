#ifndef JXTEMPLATEHTTPChannellib_H_
#define JXTEMPLATEHTTPChannellib_H_

#include "StateReport.h"
#include <vector>
#include <string>
#include "Channellib.h"

namespace smsp
{

    class JXTEMPLATEHTTPChannellib : public Channellib
    {
    public:
        JXTEMPLATEHTTPChannellib();
        virtual ~JXTEMPLATEHTTPChannellib();

        void test();
        virtual bool parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason);
        virtual bool parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse);
        virtual int adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);
        virtual int adaptEachChannelforOauth(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);
        virtual bool parseOauthResponse(std::string response, models::AccessToken& accessToken, std::string& reason);

    private:
        void generateResponse(std::string& strResponse, int flag);
    };

} /* namespace smsp */
#endif /* JXUSSDHTTPChannellib_H_ */