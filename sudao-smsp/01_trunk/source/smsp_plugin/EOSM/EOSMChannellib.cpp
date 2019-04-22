#include "EOSMChannellib.h"
#include <stdio.h>
#include <time.h>
#include "json/json.h"
#include <fstream>
#include "xml.h"

namespace smsp
{
 	extern "C"
    {
        void * create()
        {
            return new EOSMChannellib;
        }
    }

    EOSMChannellib::EOSMChannellib()
    {
    }
    EOSMChannellib::~EOSMChannellib()
    {

    }

   

    void EOSMChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool EOSMChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        //printf("%s\n", respone.data());
        try
        {
            /* BEGIN: Modified by LionGong, 2015/7/15 */
            Json::Reader reader(Json::Features::strictMode());
            Json::Value root;
            std::string js;

            if (Json::json_format_string(respone, js) < 0)
            {
                //LogError("###EOSM send-response json message error###");
                return false;
            }
            if(!reader.parse(js,root))
            {
                //LogError("[EOSMParser::parseSend] parse is failed.")
                return false;
            }
            /* END:   Modified by LionGong, 2015/7/15 */
            strReason = root["desc"].asString();
            int result = root["result"].asInt();
            if(result!=1)
                return false;
            smsid = root["taskId"].asString();
            status = "0";
            return true;
        }
        catch(...)
        {
            return false;
        }
    }

    bool EOSMChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        try
        {
            smsp::StateReport report;
            /* BEGIN: Modified by LionGong, 2015/7/15 */
            Json::Reader reader(Json::Features::strictMode());
            Json::Value root;
            std::string js;
            if (Json::json_format_string(respone, js) < 0)
            {
                //LogNotice("###EOSM report json message error###");
                return false;
            }
            if(!reader.parse(js,root))
                return false;
            /* END:   Modified by LionGong, 2015/7/15 */
            utils::UTFString utfHelper = utils::UTFString();
            std::string tmpDesc = root["desc"].asString();
            Json::Value obj;
            /* BEGIN: Modified by fuxunhao, 2015/10/14   PN:AutoConfig */
            if(!root["obj"].isNull())
            {
                obj = root["obj"];
            }
            else
            {
                return false;
            }
            /* END:   Modified by fuxunhao, 2015/10/14   PN:AutoConfig */
            /* BEGIN: Added by fuxunhao, 2015/9/16   PN:UP_MO */
            std::string::size_type pos = tmpDesc.find("MO");
            if (pos != std::string::npos)
            {
                for(unsigned int i = 0; i < obj.size(); i++)
                {
                    smsp::UpstreamSms upsms;
                    Json::Value tmpValue = obj[i];
                    std::string strContent = tmpValue["content"].asString();
                    std::string strDestaddr = tmpValue["destaddr"].asString();
                    std::string strReceiveTime = tmpValue["receivetime"].asString();
                    std::string strSourceaddr = tmpValue["sourceaddr"].asString();
                    std::string strExtcode = tmpValue["extcode"].asString();

                    ///strReceiveTime.erase(strReceiveTime.length()-2); ///2013-09-13 17:10:05.0
                    strReceiveTime = strReceiveTime.substr(0, strReceiveTime.length()-2);

                    upsms._appendid = strDestaddr;/////.substr(strDestaddr.length() - 3);
                    upsms._content = strContent;
                    upsms._phone = strSourceaddr;
                    upsms._upsmsTimeStr = strReceiveTime;
                    upsms._upsmsTimeInt = strToTime((char*)strReceiveTime.data());
                    //LogNotice("==strExtcode[%s],strContent[%s],strSourceaddr[%s],strReceiveTime[%s].",
                    //strExtcode.data(),strContent.data(),strSourceaddr.data(),strReceiveTime.data());
                    upsmsRes.push_back(upsms);

                }
                /* BEGIN: Added by fuxunhao, 2015/10/14   PN:AutoConfig */
                return true;
                /* END:   Added by fuxunhao, 2015/10/14   PN:AutoConfig */
            }
            /* END:   Added by fuxunhao, 2015/9/16   PN:UP_MO */

            for(unsigned int i = 0; i < obj.size(); i++)
            {
                Json::Value tmpValue = obj[i];
                std::string desc = tmpValue["desc"].asString();
                std::string toMobile = tmpValue["mobile"].asString();
                std::string reportTime = tmpValue["reportTime"].asString();
                int status = tmpValue["result"].asInt();
                std::string state = tmpValue["state"].asString();
                std::string smsid = tmpValue["taskId"].asString();

                /* BEGIN: Modified by fuxunhao, 2015/12/3   PN:Batch Send Sms */
                if(status!=0)	//reportResp confirm failed
                {
                    status = SMS_STATUS_CONFIRM_FAIL;
                    
                }
				else	//reportResp confirm suc
				{
					status = SMS_STATUS_CONFIRM_SUCCESS; 
				}
                /* END:   Modified by fuxunhao, 2015/12/3   PN:Batch Send Sms */
                char tmp[64] = {0};
                sprintf(tmp, "%s", reportTime.data());
                report._smsId=smsid;
                utfHelper.u2g(desc,report._desc);
                report._desc=desc;
                report._reportTime=strToTime(tmp);
                report._status=status;

                reportRes.push_back(report);
            }

            return true;
        }
        catch(...)
        {
            //LogWarn("[EOSMChannellib::parseReport] is failed.");
            return false;
        }
    }


    int EOSMChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {

        return 0;
    }

    int EOSMChannellib::getChannelInterval(SmsChlInterval& smsChIntvl)
    {
        smsChIntvl._itlValLev1 = 20;   //val for lev2, num of msg
        //smsChIntvl->_itlValLev2 = 20; //val for lev2

        smsChIntvl._itlTimeLev1 = 20;      //time for lev1, time of interval
        smsChIntvl._itlTimeLev2 = 2;       //time for lev 2
        //smsChIntvl->_itlTimeLev3 = 1;     //time for lev 3
		return 0;
    }


} /* namespace smsp */


int main()
{
    printf("Successful!");
}

