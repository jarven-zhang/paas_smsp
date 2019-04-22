#include "SHHTTPChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include "xml.h"
#include "UTFString.h"

namespace smsp
{
    extern "C"
    {
        void * create()
        {
            return new SHHTTPChannellib;
        }
    }
    
    SHHTTPChannellib::SHHTTPChannellib()
    {
    }
    SHHTTPChannellib::~SHHTTPChannellib()
    {

    }
    
    bool SHHTTPChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        ////0,12345,成功
        ////-1,0,失败原因
        try
        {   cout<<"send response:"<<respone.data()<<endl;
            std::vector<string> vecStr;
            split_Ex_Vec(respone,",",vecStr);
            int iStatus = atoi(vecStr.at(0).data());
            std::string strSmsid = vecStr.at(1);
            strReason = vecStr.at(2);

            cout<<"status:"<<iStatus<<",smsId:"<<strSmsid.data()<<",reason"<<strReason.data()<<endl;
            if (iStatus == 0)
            {
                smsid = strSmsid;
                status = "0";
            }
            else
            {
                status = "4";
            }
        }
        catch(...)
        {
            cout<<"SHHTTPChannellib::parseSend is expence."<<endl;
            return false;
        }
    }

    bool SHHTTPChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        /*
          reportRT
          act=statusreport&
          unitid=100&
          username=test&
          passwd=test&
          mession=1234&
          status=1&
          phone=18911111111

          upMO
          act=upmsg&
          unitid=100&
          username=test&
          passwd=test&
          msg=text&
          phone=13911111111&  
          spnumber=1065902051214&
          uptime=2008-01-01 12:00:00
        */
        try
        {
           cout<<"report respone:"<<respone.data()<<endl;
           std::map<string,string> mapStr;
           split_Ex_Map(respone,"&",mapStr);

           std::string strAct = mapStr["act"];
           if (0 == strAct.compare("statusreport"))
           {    
               smsp::StateReport report;
               std::string strRt = mapStr["status"];
               report._desc = strRt;
               report._reportTime = time(NULL);
               report._smsId = mapStr["mession"];
               strResponse = "0";
               if ((0 == strRt.compare("DELIVRD")) || (0 == strRt.compare("0")))
               {
                    report._status = SMS_STATUS_CONFIRM_SUCCESS;
               }
               else
               {
                    report._status = SMS_STATUS_CONFIRM_FAIL;
               }
               reportRes.push_back(report);
               return true;
           }
           else if (0 == strAct.compare("upmsg"))
           {
                smsp::UpstreamSms upReport;
                upReport._content = mapStr["msg"];
                upReport._phone = mapStr["phone"];
                upReport._upsmsTimeInt = time(NULL);
                std::string strSp = mapStr["spnumber"];
                upReport._appendid = strSp;//////.substr(strSp.length()-2);
                upsmsRes.push_back(upReport);
                return true;
           }
           else
           {
                cout<<"this is act:"<<strAct.data()<<",is invalid."<<endl;
                return false;
           }
        }
        catch(...)
        {
            cout<<"SHHTTPChannellib::parseReport is expence."<<endl;
            return false;
        }
    }


    int SHHTTPChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {

        return 0;
    }


} /* namespace smsp */
