#ifndef DCGChannellib_H_
#define DCGChannellib_H_
#include "StateReport.h"
#include <vector>
#include <string>
#include "Channellib.h"

enum{
	MAYBE_ERROR 	= -1,
	MAYBE_REPORT 	= 0,
	MAYBE_MO 		= 1,
	MAYBE_NULL		= 2,
};

namespace smsp
{

    class DCGChannellib:public Channellib
    {
    public:
        DCGChannellib();
        virtual ~DCGChannellib();

        void test();
        virtual bool parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason);
        virtual bool parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse);
        virtual int adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);

	private:
		void string_replace(string&s1, const string&s2, const string&s3);
		void splitExMap(std::string& strData, std::string strDime, std::map<std::string,std::string>& mapSet);
    };

} /* namespace smsp */
#endif /* TESTINPARSER_H_ */