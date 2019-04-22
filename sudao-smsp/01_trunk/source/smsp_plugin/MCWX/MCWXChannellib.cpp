#include "MCWXChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include "xml.h"

namespace smsp
{

    MCWXChannellib::MCWXChannellib()
    {
    }
    MCWXChannellib::~MCWXChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new MCWXChannellib;
        }
    }
    
    bool MCWXChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        try
        {
            std::string::size_type pos = respone.find_first_of("<");
            respone = respone.substr(pos);
            respone.erase(respone.find_last_of(">") + 1);
            cout<<"==========MCWX ParseSend:"<<respone.data()<<endl;

            
            TiXmlDocument myDocument;
            if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
            {
                cout<<"MCWXChannellib::parseSend parse is failed."<<endl;
                return false;
            }

            TiXmlElement* rootEle = myDocument.RootElement();
            if(NULL==rootEle)
            {
                cout<<"MCWXChannellib::parseSend rootEle is NULL."<<endl;
                return false;
            }

            const string* pStrReturn = rootEle->Attribute(string("return"));
            if (NULL == pStrReturn)
            {
                cout<<"MCWXChannellib::parseSend pStrReturn is NULL."<<endl;
                return false;
            }

            cout<<"======1=========return:"<<pStrReturn->data()<<"length:"<<pStrReturn->length()<<endl;

            if (0 != strcmp("0",pStrReturn->data()))
            {
                cout<<"=====is not 0===="<<endl;
                const string* pStrInfo = rootEle->Attribute(string("info"));
                if (NULL == pStrInfo)
                {
                    return false;
                }

                strReason = pStrInfo->data();
                return false;
            }
            
            TiXmlElement* respEle = rootEle->FirstChildElement();
            if(NULL==respEle)
            {
                cout<<"MCWXChannellib::parseSend respEle is NULL."<<endl;
                return false;
            }

            const string* pStrMsgid = respEle->Attribute(string("msgid"));
            if (NULL == pStrMsgid)
            {
                cout<<"MCWXChannellib::parseSend pStrMsgid is NULL."<<endl;
                return false;
            }

            const string* pReturn = respEle->Attribute(string("return"));
            if (NULL == pReturn)
            {
                cout<<"MCWXChannellib::parseSend pReturn is NULL."<<endl;
                return false;
            }

            const string* pInfo = respEle->Attribute(string("info"));
            if(NULL == pInfo)
            {
                cout<<"MCWXChannellib::parseSend pInfo is NULL."<<endl;
                return false;
            }

            cout<<"=====2========return:"<<pReturn->data()<<endl;

            if (0 != strcmp("0",pReturn->data()))
            {
                status = "1";
            }
            else
            {
                status = "0";
            }

            smsid = pStrMsgid->data();
            strReason = pInfo->data();

            return true;
        }
        catch(...)
        {
            return false;
        }
    }

    bool MCWXChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        try
        {
            std::string::size_type pos = respone.find_first_of("<");
            respone = respone.substr(pos);
            respone.erase(respone.find_last_of(">") + 1);
            cout<<"==========MCWX parseReport:"<<respone.data()<<endl;

            TiXmlDocument myDocument;
            if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
            {
                return false;
            }

            TiXmlElement* rootEle = myDocument.RootElement();
            if(NULL==rootEle)
            {
                return false;
            }

            const string* pStrReturn = rootEle->Attribute(string("return"));
            if (NULL == pStrReturn)
            {
                return false;
            }

            if (0 != strcmp("0",pStrReturn->data()))
            {
                return false;
            }
            
            TiXmlElement* chEle = rootEle->FirstChildElement();

            while(NULL != chEle)
            {
                std::string type;
                type = chEle->Value();
                if("report" == type)
                {
                    const string* pStrMsgid = chEle->Attribute(string("msgid"));
                    if (NULL == pStrMsgid)
                    {
                        return false;
                    }

                    const string* pStrStatus = chEle->Attribute(string("status"));
                    if (NULL == pStrStatus)
                    {
                        return false;
                    }

                    const string* pInfo = chEle->Attribute(string("info"));
                    if (NULL == pInfo)
                    {
                        return false;
                    }

                    smsp::StateReport report;
                    report._desc = pInfo->data();
                    report._smsId = pStrMsgid->data();
                    report._reportTime = time(NULL);
                    if (0 == strcmp("0",pStrStatus->data()))
                    {
                        report._status = SMS_STATUS_CONFIRM_SUCCESS;
                    }
                    else
                    {
                        report._status = SMS_STATUS_CONFIRM_FAIL;
                    }

                    reportRes.push_back(report);
                   
                }
                else if ("upmsg" == type)
                {
                    const string* pSpNumber = chEle->Attribute(string("spnumber"));
                    if (NULL == pSpNumber)
                    {
                        return false;
                    }

                    const string* pPhone = chEle->Attribute(string("phone"));
                    if (NULL == pPhone)
                    {
                        return false;
                    }

                    const string* pContent = chEle->Attribute(string("msgcontent"));
                    if (NULL == pContent)
                    {
                        return false;
                    }

                    const string* pTime = chEle->Attribute(string("motime"));
                    if (NULL == pTime)
                    {
                        return false;
                    }

                    smsp::UpstreamSms up;
                    string strSpNumber(pSpNumber->data());
                    if (strSpNumber.length() < 6)
                    {
                        return false;
                    }

                    strSpNumber = strSpNumber;/////.substr(strSpNumber.length()-6);
                    up._appendid = strSpNumber;
                    up._content = pContent->data();
                    up._phone = pPhone->data();

                    string strTime(pTime->data());
                    std::string::size_type pos = strTime.find(".0");
                    if (pos != std::string::npos)
                    {
                        strTime = strTime.substr(0,pos);
                    }
                    
                    up._upsmsTimeStr = strTime;
                    up._upsmsTimeInt = strToTime((char*)strTime.data());
                    upsmsRes.push_back(up);
                    
                }
                else
                {
                    cout<<"this is invalid type:"<<type.data()<<endl;
                    return false;
                }
                
                chEle = chEle->NextSiblingElement();
            }
           
            return true;
        }
        catch(...)
        {
            return false;
        }
    }


    int MCWXChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
        std::string::size_type pos = data->find("%contentEx%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%contentEx%"), sms->m_strContent);
		}
        return 0;
    }


} /* namespace smsp */
