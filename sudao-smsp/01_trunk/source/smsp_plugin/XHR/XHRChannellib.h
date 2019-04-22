#ifndef XHRChannellib_H_
#define XHRChannellib_H_
#include "StateReport.h"
#include <vector>
#include <map>
#include <string>
#include "Channellib.h"

namespace smsp
{

    class XHRChannellib:public Channellib
    {
    public:
        XHRChannellib();
        virtual ~XHRChannellib();

        virtual bool parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason);
        virtual bool parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse);
        virtual int adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);

    private:
        void split_flag_Map(const std::string& strData, std::map<std::string,std::string>& mapSet);
        std::string encode(const std::string& str);
        std::string urlDecode(const std::string& str);
    };

} /* namespace smsp */
#endif /* GDPARSER_H_ */
