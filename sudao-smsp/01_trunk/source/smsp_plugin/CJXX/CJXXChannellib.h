#ifndef CJXXChannellib_H_
#define CJXXChannellib_H_
#include "StateReport.h"
#include "Channellib.h"

namespace smsp
{

    class CJXXChannellib:public Channellib
    {
    public:
        CJXXChannellib();
        virtual ~CJXXChannellib();

        void test();
        virtual bool parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason);
        virtual bool parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse);
        virtual int  adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);

	private:
		void splitExVectorSkipEmptyString(std::string& strData, std::string strDime, std::vector<std::string>& vecSet);
    };

} /* namespace smsp */
#endif /* TESTINPARSER_H_ */