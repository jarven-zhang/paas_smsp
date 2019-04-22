#include "EMASYMChannellib.h"
#include <stdio.h>
#include <time.h>
#include "json/json.h"
#include <fstream>
#include "xml.h"
#include "aes_Code.h"

namespace smsp
{
 	extern "C"
    {
        void * create()
        {
            return new EMASYMChannellib;
        }
    }

    EMASYMChannellib::EMASYMChannellib()
    {
    }
    EMASYMChannellib::~EMASYMChannellib()
    {

    }

    bool EMASYMChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        try
        {
            ////cout<<"start ==R==== respone:"<<respone.data()<<endl;
            Json::Reader reader(Json::Features::strictMode());
            Json::Value root;
            std::string js;
            if (Json::json_format_string(respone, js) < 0)
            {
                return false;
            }
            if(!reader.parse(js,root))
            {
                return false;
            }

            smsid = root["sms_id"].asString();

            int iSuccess = root["success_number"].asInt();
            int iFail = root["fail_number"].asInt();
            if (1 == iSuccess)
            {
                status = "0";
                strReason = "";
            }
            else if (1 == iFail)
            {
                Json::Value obj = root["fail_mobiles"];

                for (int i = 0;i < obj.size();i++)
                {
                    Json::Value tmpValue = obj[i];
                    strReason = tmpValue["fail_code"].asString();
                }
                
                
                status = "4";
            }
            else
            {
                status = "4";
            }

            return true;
        }
        catch(...)
        {
            return false;
        }
    }

    bool EMASYMChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        try
        {   
            ////cout<<"start ===RT===:"<<respone.data()<<endl;
            
            if (respone.length() < 10)
            {
                return false;
            }
               
            Json::Reader reader(Json::Features::strictMode());
            Json::Value root;
            std::string js;
            if (Json::json_format_string(respone, js) < 0)
            {
                return false;
            }
            if(!reader.parse(js,root))
            {
                return false;
            }

            Json::Value obj;
            if(!root["reports"].isNull())
            {
                obj = root["reports"];
            }
            else
            {
                return false;
            }

            for (int i = 0; i < obj.size(); i++)
            {   
                
                Json::Value tmpValue = obj[i];

                smsp::StateReport report;
                report._smsId = tmpValue["sms_id"].asString();
                report._desc = tmpValue["message"].asString();
                string strState = tmpValue["code"].asString();
                if ("SUCCESS" == strState)
                {
                    report._status = SMS_STATUS_CONFIRM_SUCCESS;
                }
                else
                {
                    report._status = SMS_STATUS_CONFIRM_FAIL;
                }
                report._reportTime = time(NULL);
                reportRes.push_back(report);
            }
            strResponse = "SUCCESS";
            return true;
        }
        catch(...)
        {
            return false;
        }
    }


    int EMASYMChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
        /**

        postDat 格式如下:
        public_key=EMAS-9006-752D-E6DC&sign=%aes%&&sms_data=%jsoncontent%&private_key=6752DE6DC74A4F5F
        
        jsoncontent数据格式如下 是json
        {"mobiles":["15538853919","15673648362"],"smscontent":"【清华大学】 i love you ! "}
        */

        std::string::size_type pos = data->find("&private_key=");
        if (pos == std::string::npos)
        {
            ////cout<<"data format is error:"<<data->data()<<endl;
            return -1;
        }

        string strKey = data->substr(pos+strlen("&private_key="));
        data->erase(pos);

        

        string strData = "";
        strData.append("{\"mobiles\":[\"");    
        strData.append(sms->m_strPhone);
        strData.append("\"],\"smscontent\":\"");
        strData.append(sms->m_strContent);
        strData.append("\"}");

        /////cout<<"strData:"<<strData.data()<<endl;
        

        UInt64 uNow = time(NULL);
        char pSignTime[64] = {0};
		struct tm ptm = {0};
		localtime_r((time_t*)&uNow,&ptm);
        strftime(pSignTime,sizeof(pSignTime),"%Y%m%d%H%M%S000",&ptm);
        string strSignTime(pSignTime);

        unsigned char key[16] = {0};
    	for (int i = 0; i < 16; i++)
    	{
    		key[i] = strKey[i];
    	}

        AES aes(key);
        int iLenSec = strSignTime.length(); 
    	int iNewLen = 0;
    	int iBuLen = 0;

    	if (iLenSec < 16)
    	{
    		iNewLen = 16;
    		iBuLen = 16 - iLenSec;
    	}
    	else if (0 == (iLenSec % 16))
    	{
    		iNewLen = iLenSec + 16;
    		iBuLen = 16;
    	}
    	else
    	{
    		iNewLen = ((iLenSec + 16) / 16) * 16;
    		iBuLen = iNewLen - iLenSec;
    	}

    	unsigned char* pIn = new unsigned char[iNewLen];
    	memset(pIn, 0, iNewLen);
    	for (int i = 0; i < strSignTime.length(); i++)
    	{
    		pIn[i] = strSignTime[i];
    	}

    	for (int i = iLenSec; i < iNewLen; i++)
    	{
    		pIn[i] = iBuLen;
    	}
        
    	unsigned char* pTmp = pIn;

    	int en_len = 0;
    	while (en_len < iNewLen)
    	{
    		aes.Cipher(pTmp);
    		pTmp += 16;
    		en_len += 16;
    	}

        string strHex = "";
    	std::string HEX_CHARS = "0123456789ABCDEF";

    	for (int i = 0; i < iNewLen; i++)
    	{
    		strHex.append(1, HEX_CHARS.at(pIn[i] >> 4 & 0x0F));
    		strHex.append(1, HEX_CHARS.at(pIn[i] & 0x0F));
    	}
        delete[] pIn;
        pIn = NULL;
        ////cout<<"strHex:"<<strHex.data()<<endl;

        
        string strJsonContent = smsp::Channellib::urlEncode(strData);


        pos = data->find("%jsoncontent%");
        if (pos == std::string::npos)
        {
            /////cout<<"data format is error:"<<data->data()<<endl;
            return -1;
        }
        data->replace(pos, strlen("%jsoncontent%"), strJsonContent);

        pos = data->find("%aes%");
        if (pos == std::string::npos)
        {
            /////cout<<"data format is error:"<<data->data()<<endl;
            return -1;
        }
        data->replace(pos, strlen("%aes%"),strHex);
    
        ////cout<<"data:"<<data->data()<<endl;    
        return 0;
    }

    int EMASYMChannellib::getChannelInterval(SmsChlInterval& smsChIntvl)
    {
        smsChIntvl._itlValLev1 = 20;   //val for lev2, num of msg
        //smsChIntvl->_itlValLev2 = 20; //val for lev2

        smsChIntvl._itlTimeLev1 = 20;      //time for lev1, time of interval
        smsChIntvl._itlTimeLev2 = 2;       //time for lev 2
        //smsChIntvl->_itlTimeLev3 = 1;     //time for lev 3
		return 0;
    }


} /* namespace smsp */
