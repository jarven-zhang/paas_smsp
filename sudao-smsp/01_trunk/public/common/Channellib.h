#ifndef __CHANNELLIB_H__
#define __CHANNELLIB_H__

#include "platform.h"
#include <vector>
#include <map>
#include <string>
#include "StateReport.h"
#include "UpstreamSms.h"
#include "SmsContext.h"
#include "StateResponse.h"

#include "UTFString.h"
#include "SmsChlInterval.h"
#include "ssl/md5.h"
#include "ErrorCode.h"
#include <string.h>
#include "../models/AccessToken.h"


namespace smsp
{
    class Channellib
    {
        typedef void  (*REGCALLBACK)(std::vector<smsp::StateReport>& reportRes, std::vector<smsp::UpstreamSms>& upsmsRes);

    public:
        Channellib();
        virtual ~Channellib();
        virtual long strToTime(char *str);
        virtual void split(char *str, const char *delim, std::vector<char*> &list);

        /* BEGIN: Added by fuxunhao, 2015/11/18   PN:Batch Send Sms */
        virtual void split_Ex_Map(std::string& strData, std::string strDime, std::map<std::string,std::string>& mapSet);
        virtual void split_Ex_Vec(std::string& strData, std::string strDime, std::vector<std::string>& vecSet);
        /* END:   Added by fuxunhao, 2015/11/18   PN:Batch Send Sms */

        void getMD5ForZY(string& strAppKey, string& strPhone, string& strUid, string& strContent, Int64 iSendtime, string& strSign);

		int getChannelInterval(SmsChlInterval& smsChIntvl);
		
		//virtual bool Init(const std::string config_path){return true;};
		/*支持响应回来多个请求*/
        virtual bool parseSend(std::string respone, std::string& smsid, std::string& status, std::string& strReason)=0;
        /* BEGIN: Modified by fuxunhao, 2015/11/18   PN:Batch Send Sms */
        virtual bool parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)=0;
        /* END:   Modified by fuxunhao, 2015/11/18   PN:Batch Send Sms */
        virtual int adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)=0;
		
        virtual std::string getTmpSmsId();      //??? what is this
        static UInt32 _sn;
        static unsigned char toHex(unsigned char x);
        static unsigned char fromHex(unsigned char x);
        static std::string urlEncode(const std::string& str);
        static std::string urlDecode(const std::string& str);

        static std::string encodeBase64(const char *data, UInt32 length);
        static std::string encodeBase64(const std::string &data);
        static std::string decodeBase64(const char *data, UInt32 length);
        static std::string decodeBase64(const std::string &data);

		std::string CalMd5(const std::string str);
		void splitExVectorSkipEmptyString(std::string& strData, std::string strDime, std::vector<std::string>& vecSet);
		void string_replace(string&s1, const string&s2, const string&s3);

        virtual int adaptEachChannelforOauth(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
        {
            return 0;
        }
        
        virtual bool parseOauthResponse(std::string response, models::AccessToken& accessToken, std::string& reason)
        {
            return true;
        }
		
		
		/*默认不处理多值响应*/	
		virtual bool parseSendMutiResponse(std::string respone, smsp::mapStateResponse &responses, std::string& strReason )
		{
			return false;
		}
		
    };

}/* namespace smsp */
#endif /* __CHANNELLIB_H__ */
