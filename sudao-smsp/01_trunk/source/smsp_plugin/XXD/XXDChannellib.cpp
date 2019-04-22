#include "XXDChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include "xml.h"
#include "UTFString.h"
#include <map>

namespace smsp
{
    extern "C"
    {
        void * create()
        {
            return new XXDChannellib;
        }
    }
    
    XXDChannellib::XXDChannellib()
    {
    }
    XXDChannellib::~XXDChannellib()
    {

    }

    void XXDChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

	
	///mylib->parseSend(strData, pSms->m_strChannelSmsId, state, strReason);
    bool XXDChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
	 /*
	 <?xml version="1.0"  encoding="utf-8" ?>
	 <returnsms>
		 <returnstatus>Success/Failed</returnstatus>
		 <message>ok/"用户名或密码不存�?</message>
		 <remainpoint>4</remainpoint>//余额
		 <taskID>710696</taskID>//返回本次任务的序列ID
		 <successCounts>3</successCounts> //成功短信数：当成功后返回提交成功短信�?
	 </returnsms>
	 */
		try
 		{
			respone = smsp::Channellib::urlDecode(respone);
			TiXmlDocument myDocument;
			TiXmlHandle hRoot(0);
            if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
            {  
            	std::cout<<"XXDChannellib.cpp:55 parseSend() faild! "<<std::endl;
                return false;
            }

			TiXmlElement *pElem = myDocument.RootElement();
			if (NULL == pElem || strcmp(pElem->Value(), "returnsms") != 0)
			{
				std::cout << "XXDChannellib.cpp:62 parseSend() faild! " << std::endl;
				return false;
			}
			
			hRoot = TiXmlHandle(pElem);
			pElem = hRoot.FirstChild().Element();
			std::map<std::string, std::string> submitMap;
			for (pElem; pElem; pElem = pElem->NextSiblingElement())
			{
				const char *pKey=pElem->Value();
				const char *pText=pElem->GetText();
				if (pKey && pText) 
				{
					submitMap[pKey] = pText;
				}
			}

			if(submitMap["returnstatus"].compare("Success") == 0)
			{
				status.assign("0");	
			}
			else
			{
				status.assign(submitMap["returnstatus"]);
			}
			
			strReason.assign(submitMap["message"]);
			smsid.assign(submitMap["taskID"]);
			return true	;	
 		}
        catch(...)
        {
           std::cout<<"XYCChannellib.cpp:127 parseSend() faild! "<<std::endl;        
           return false;
        }
    }

    bool XXDChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
		/*<?xml version="1.0" encoding="UTF-8"?> 
		<returnsms>
			<statusbox>
				<mobile>15023239810</mobile>-------------��Ӧ���ֻ�����
				<taskid>1212</taskid>-------------ͬһ������ID
				<status>10</status>---------״̬����----10�����ͳɹ���20������ʧ��
				<receivetime>2011-12-02 22:12:11</receivetime>-------------����ʱ��
				<errorcode>DELIVRD</errorcode>-�ϼ����ط���ֵ����ͬ���ط���ֵ��ͬ������Ϊ�ο�
				<extno>01</extno>--�Ӻţ����Զ�����չ��
			</statusbox>
			<callbox>
				<mobile>15023239810</mobile>-------------��Ӧ���ֻ�����
				<taskid>1212</taskid>-------------ͬһ������ID
				<content>��ã��Ҳ���Ҫ</content>---------��������
				<receivetime>2011-12-02 22:12:11</receivetime>-------------����ʱ��
				<extno>01</extno>----�Ӻţ����Զ�����չ��
			</callbox>			
		</returnsms>
		*/
		int pos = respone.find_last_of('>');
		string tempStr = respone.substr(0,pos+1);
		TiXmlDocument myDocument;
		TiXmlHandle hRoot(0);
		try
		{
            strResponse.assign("ok");	
			tempStr = smsp::Channellib::urlDecode(tempStr);		
            if(myDocument.Parse(tempStr.data(),0,TIXML_DEFAULT_ENCODING))
            { 
            	std::cout<<"XXDChannellib.cpp:166 parseReport() faild! "<<std::endl;            
                return false;
            }			
			TiXmlElement *pElem = myDocument.RootElement();
            if(NULL == pElem || strcmp(pElem->Value(), "returnsms") != 0)
            {
            	std::cout<<"XXDChannellib.cpp:172 not returnsms! "<<std::endl; 
                return false;
            }
			hRoot = TiXmlHandle(pElem);
			int i = 0;
			while (true)
			{
				TiXmlElement* child = hRoot.Child("statusbox", i).ToElement();
				if (!child)
					break;
				std::map<std::string, std::string> reportMap;
				pElem = child->FirstChildElement();
				for( pElem; pElem; pElem=pElem->NextSiblingElement())
				{
					const char *pKey=pElem->Value();
					const char *pText=pElem->GetText();
					if (pKey && pText) 
					{
						reportMap[pKey] = pText;
					}
				}
				smsp::StateReport report;
				report._phone = reportMap["mobile"];
				report._smsId = reportMap["taskid"];
				if(reportMap["status"].compare("10") == 0)
				{
					report._status = SMS_STATUS_CONFIRM_SUCCESS;
				}
				else
				{
					report._status = SMS_STATUS_CONFIRM_FAIL;
				}
				report._reportTime= strToTime(const_cast<char *>(reportMap["receivetime"].data()));
				report._desc = 	reportMap["errorcode"];
				reportRes.push_back(report);
				i++;
				
			}
			i = 0;
			while (true)
			{
				TiXmlElement* child = hRoot.Child("callbox", i).ToElement();
				if (!child)
					break;
				std::map<std::string, std::string> upstreamMap;
				pElem = child->FirstChildElement();
				//pElem = hRoot.FirstChild("callbox").FirstChild().Element();
				for( pElem; pElem; pElem=pElem->NextSiblingElement())
				{
					const char *pKey=pElem->Value();
					const char *pText=pElem->GetText();
					if (pKey && pText) 
					{
						upstreamMap[pKey] = pText;
					}
				}
				smsp::UpstreamSms upstream;
				upstream._phone= upstreamMap["mobile"];
				upstream._content = upstreamMap["content"];
				upstream._upsmsTimeStr = upstreamMap["receivetime"];
				upstream._upsmsTimeInt= strToTime(const_cast<char *>(upstream._upsmsTimeStr.data()));
				upstream._appendid = upstreamMap["extno"];			
				upsmsRes.push_back(upstream);
				i++;
			}
		}						
        catch(...)
        {
            cout<<"XXDChannellib::parseReport alert";
            return false;
        }
    }


    int XXDChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
		vhead->push_back("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
		//cout<<"-------adaptEachChannel------------data="<<*data<<endl;
		
		return 0;
    }


} /* namespace smsp */
