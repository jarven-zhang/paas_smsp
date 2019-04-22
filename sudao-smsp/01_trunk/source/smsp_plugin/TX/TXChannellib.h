#ifndef TXCHANNELLIB_H_
#define TXCHANNELLIB_H_

#include "StateReport.h"
#include <vector>
#include <string>
#include "Channellib.h"


namespace smsp
{

    class TXChannellib:public Channellib
    {
    public:
        TXChannellib();
        virtual ~TXChannellib();
        virtual bool parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason);
        virtual bool parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse);
        virtual int adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);
//        std::string Encode(const std::string& str);
    private:
//        int signatureMoveToTail(string& destSrc);
//        std::map<string,string> m_mapErrorCode;


    };

} /* namespace smsp */
#endif
