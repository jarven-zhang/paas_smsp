#ifndef DXTEMPLATEChannellib_H_
#define DXTEMPLATEChannellib_H_

#include "StateReport.h"
#include <vector>
#include <string>
#include "Channellib.h"

namespace smsp
{

    class DXTEMPLATEChannellib : public Channellib
    {
    public:
        DXTEMPLATEChannellib();
        virtual ~DXTEMPLATEChannellib();

        void test();
        virtual bool parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason);
        virtual bool parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse);
        virtual int adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);
    };

} /* namespace smsp */
#endif /* JXUSSDHTTPChannellib_H_ */