#include "CXDYChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include "xml.h"

namespace smsp 
{

CXDYChannellib::CXDYChannellib() 
{
}
CXDYChannellib::~CXDYChannellib() 
{

}

extern "C"
{
    void * create()
    {
        return new CXDYChannellib;
    }
}

bool CXDYChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
{	
	/*
	<?xml version="1.0" encoding="utf-8"?>
	<responsedata>
		<resultcode>000</resultcode>
		<resultdesc>操作成功</resultdesc>
		<return>
			<subretcode>0</subretcode>
			<smssendcode>0<smssendcode>
			<smsid>5412854568565869<smsid>
		</return>
	</responsedata>
	*/
	
	try
	{
		respone = respone.substr(respone.find_first_of("<"));
		respone.erase(respone.find_last_of(">") + 1);
	
		TiXmlDocument myDocument;

		if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
		{
			//std::cout << "Parse failed" << std::endl;
			return false;
		}

		TiXmlElement *element = myDocument.RootElement();
		if(NULL==element)
		{
			//std::cout << "element is null" << std::endl;
			return false;
		}

		TiXmlElement *resultcodeElement = element->FirstChildElement();		//resultcode element
		if(NULL==resultcodeElement)
		{	
			//std::cout << "resultcodeElement is null" << std::endl;
			return false;
		}
		TiXmlNode *resultcodeNode = resultcodeElement->FirstChild();
		if(NULL==resultcodeNode)
		{
            //std::cout << "resultcodeNode is NULL." << std::endl;
            return false;
        }
	
		TiXmlElement *resultdescElement = resultcodeElement->NextSiblingElement();	//resultdesc element
		if(NULL==resultdescElement)
		{	
			//std::cout << "resultdescElement is null" << std::endl;
			return false;
		}
		TiXmlNode *resultdescNode = resultdescElement->FirstChild();
		if(NULL==resultdescNode)
		{
            //std::cout << "resultdescNode is NULL." << std::endl;
            ///return false;
        }

		std::string str_resultcode = resultcodeNode->Value();
		std::string str_resultdesc = resultdescNode->Value();
	
		TiXmlElement *returnElement = resultdescElement->NextSiblingElement();	//return node
		if(NULL==returnElement)
		{	
			//std::cout << "returnElement is null" << std::endl;
			status = "4";
			strReason = str_resultdesc;	
			return true;
		}

		//subretcode
		TiXmlElement *subretcodeElement = returnElement->FirstChildElement();
		if(NULL==subretcodeElement)
		{
            //std::cout << "subretcodeElement is NULL." << std::endl;
            return false;
        }
		TiXmlNode *subretcodeNode = subretcodeElement->FirstChild();
		if(NULL==subretcodeNode)
		{
            //std::cout << "subretcodeNode is NULL." << std::endl;
            return false;
        }

		//smssendcode
		TiXmlElement *smssendcodeElement = subretcodeElement->NextSiblingElement();
		if(NULL==smssendcodeElement)
		{
            //std::cout << "smssendcodeElement is NULL." << std::endl;
            return false;
        }
		TiXmlNode *smssendcodeNode = smssendcodeElement->FirstChild();
		if(NULL==smssendcodeNode)
		{
            //std::cout << "smssendcodeNode is NULL." << std::endl;
            return false;
        }

		//smsid
		TiXmlElement *smsidElement = smssendcodeElement->NextSiblingElement();
		if(NULL==smsidElement)
		{
            //std::cout << "smsidElement is NULL." << std::endl;
            return false;
        }
		TiXmlNode *smsidNode = smsidElement->FirstChild();
		if(NULL==smsidNode)
		{
            //std::cout << "smsidNode is NULL." << std::endl;
            return false;
        }
		
		std::string str_smsid = smsidNode->Value();
		std::string str_smssendcode = smssendcodeNode->Value();
		
		if(str_resultcode == "000" && str_smssendcode == "0")
		{
			status = "0";
		}
		else
		{
			status = "4";
			
		}
		
		smsid = str_smsid;
		strReason.assign(str_resultdesc).append("smssendcode:").append(str_smssendcode);
		
		return true;
	}
	catch(...)
	{
		//std::cout << "[CXDYChannellib::parseSend] is failed." << std::endl;
		return false;
	}
}

bool CXDYChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse) 
{	
/*
<?xml version="1.0" encoding="utf-8"?>
<root>
<seqno>2771156304</seqno>
<smsnum>3</smsnum>
<responseData>
	<MsgId>2771156304</MsgId>
    <SmsMsg>Delived</SmsMsg>
    <SendStatus>0</SendStatus>
    <ReceiveNum>13512345678</ReceiveNum>
</responseData>
<responseData>
	<MsgId>2771156305</MsgId>
    <SmsMsg>Delived</SmsMsg>
    <SendStatus>0</SendStatus>
    <ReceiveNum>13512345688</ReceiveNum>
</responseData>
<responseData>
	<MsgId>2771156308</MsgId>
    <SmsMsg>Delived</SmsMsg>
    <SendStatus>0</SendStatus>
    <ReceiveNum>13512345699</ReceiveNum>
</responseData>
</root>
*/

/*
<?xml version="1.0" encoding="utf-8"?>
<root>
<BaseNumber>10658139990008</BaseNumber>
<responseData>
	<SpNumber>106581399900089700</SpNumber>
	<UserNumber>13578**8574</UserNumber>
	<MoMsg>好的，谢谢</MoMsg>
	<MoTime>2016-11-23 07:50:16</MoTime>
	<timestamp>1479858616711</timestamp>
</responseData>
</root>
*/
	try
	{
		respone = respone.substr(respone.find_first_of("<"));
		respone.erase(respone.find_last_of(">") + 1);
	
		TiXmlDocument myDocument;

		if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
		{
            //std::cout << "myDocument.Parse is failed." << std::endl;
			return false;
		}

		TiXmlElement *element = myDocument.RootElement();
		if(NULL==element)
        {
            //std::cout << "element is null" << std::endl;
			return false;
		}

        TiXmlElement* pSeqNo = element->FirstChildElement();
		if(NULL==pSeqNo)
        {
            //std::cout << "pSeqNo is null" << std::endl;
			return false;
		}

		std::string type = pSeqNo->Value();
		if (type == "seqno")
		{
			TiXmlElement* pSmsNum = pSeqNo->NextSiblingElement();
	        if(NULL==pSmsNum)
	        {
	            //std::cout << "pSmsNum is null" << std::endl;
				return false;
			}

			TiXmlElement *responseDataElement = pSmsNum->NextSiblingElement();	//responseDataElement
			if(NULL==responseDataElement)
			{	
				//std::cout << "responseDataElement is null" << std::endl;
				return false;
			}

			while(responseDataElement != NULL)
			{	

				TiXmlNode *MsgIdNode = NULL;
				TiXmlNode *SendStatusNode = NULL;
				TiXmlNode *ReceiveNumNode = NULL;
				TiXmlNode *SmsMsgNode = NULL;
			

	            TiXmlElement *elementOne = responseDataElement->FirstChildElement();
				if(NULL==elementOne)
				{
	                return false;
	            }
				std::string typeOne = elementOne->Value();
				////cout<<"typeOne:"<<typeOne.data()<<endl;
				if ("MsgId" == typeOne)
				{
					MsgIdNode = elementOne->FirstChild();
				}
				else if ("SendStatus" == typeOne)
				{
					SendStatusNode = elementOne->FirstChild();
				}
				else if ("ReceiveNum" == typeOne)
				{
					ReceiveNumNode = elementOne->FirstChild();
				}
				else if ("SmsMsg" == typeOne)
				{
					SmsMsgNode = elementOne->FirstChild();
				}
				else
				{
					;
				}

				TiXmlElement *elementTwo = elementOne->NextSiblingElement();
				if(NULL==elementTwo)
				{
	                return false;
	            }
				std::string typeTwo = elementTwo->Value();
				////cout<<"typeTwo:"<<typeTwo.data()<<endl;
				if ("MsgId" == typeTwo)
				{
					MsgIdNode = elementTwo->FirstChild();
				}
				else if ("SendStatus" == typeTwo)
				{
					SendStatusNode = elementTwo->FirstChild();
				}
				else if ("ReceiveNum" == typeTwo)
				{
					ReceiveNumNode = elementTwo->FirstChild();
				}
				else if ("SmsMsg" == typeTwo)
				{
					SmsMsgNode = elementTwo->FirstChild();
				}
				else
				{
					;
				}


				TiXmlElement *elementThree = elementTwo->NextSiblingElement();
				if(NULL==elementThree)
				{
	                return false;
	            }
				std::string typeThree = elementThree->Value();
				////cout<<"typeThree:"<<typeThree.data()<<endl;
				if ("MsgId" == typeThree)
				{
					MsgIdNode = elementThree->FirstChild();
				}
				else if ("SendStatus" == typeThree)
				{
					SendStatusNode = elementThree->FirstChild();
				}
				else if ("ReceiveNum" == typeThree)
				{
					ReceiveNumNode = elementThree->FirstChild();
				}
				else if ("SmsMsg" == typeThree)
				{
					SmsMsgNode = elementThree->FirstChild();
				}
				else
				{
					;
				}


				TiXmlElement *elementFour = elementThree->NextSiblingElement();
				if(NULL==elementFour)
				{
	                return false;
	            }
				std::string typeFour = elementFour->Value();
				////cout<<"typeFour:"<<typeFour.data()<<endl;
				if ("MsgId" == typeFour)
				{
					MsgIdNode = elementFour->FirstChild();
				}
				else if ("SendStatus" == typeFour)
				{
					SendStatusNode = elementFour->FirstChild();
				}
				else if ("ReceiveNum" == typeFour)
				{
					ReceiveNumNode = elementFour->FirstChild();
				}
				else if ("SmsMsg" == typeFour)
				{
					SmsMsgNode = elementFour->FirstChild();
				}
				else
				{
					;
				}


				TiXmlElement *elementFive = elementFour->NextSiblingElement();
				if(NULL==elementFive)
				{
	                return false;
	            }
				std::string typeFive = elementFive->Value();
				////cout<<"typeFive:"<<typeFive.data()<<endl;
				if ("MsgId" == typeFive)
				{
					MsgIdNode = elementFive->FirstChild();
				}
				else if ("SendStatus" == typeFive)
				{
					SendStatusNode = elementFive->FirstChild();
				}
				else if ("ReceiveNum" == typeFive)
				{
					ReceiveNumNode = elementFive->FirstChild();
				}
				else if ("SmsMsg" == typeFive)
				{
					SmsMsgNode = elementFive->FirstChild();
				}
				else
				{
					;
				}


				std::string ReceiveTime;
				std::string SmsMsg;
				std::string MsgId;
				std::string SendStatus;

				////TiXmlNode *ReceiveTimeNode = ReceiveTimeElement->FirstChild();
				////TiXmlNode *SmsMsgNode = SmsMsgElement->FirstChild();
				
				smsp::StateReport report;
				
				/////if(NULL !=ReceiveTimeNode && NULL!= SmsMsgNode && NULL != MsgIdNode && NULL != SendStatusNode && NULL != ReceiveNumNode)
				if((NULL != MsgIdNode) && (NULL != SendStatusNode) && (NULL != ReceiveNumNode))
				{
		           	report._reportTime = time(NULL);
					report._desc = SmsMsgNode->Value();
					
					report._smsId = MsgIdNode->Value();
					report._phone = ReceiveNumNode->Value();
					SendStatus = SendStatusNode->Value();

					if(SendStatus == "0")	//reportResp confirm suc
					{
						report._status = SMS_STATUS_CONFIRM_SUCCESS;
					}
					else	//reportResp confirm failed
					{
						report._status = SMS_STATUS_CONFIRM_FAIL;
					}
				
					reportRes.push_back(report);	
	                
		        }
				else
				{
					//std::cout << "report data error" << std::endl;
		            return false;
				}
					
				responseDataElement = responseDataElement->NextSiblingElement();//下一个节点
			}
		}
		else if (type == "BaseNumber")
		{
			TiXmlElement *responseDataElement = pSeqNo->NextSiblingElement();	//responseDataElement
			if(NULL==responseDataElement)
			{	
				//std::cout << "responseDataElement is null" << std::endl;
				return false;
			}

			TiXmlNode* BaseNumberNode = pSeqNo->FirstChild();
			if (NULL == BaseNumberNode)
			{
				//std::cout << "BaseNumberNode is null" << std::endl;
				return false;
			}
			
			std::string BaseNamber = BaseNumberNode->Value();

			while(responseDataElement != NULL)
			{	

	            TiXmlElement *SpNumberElement = responseDataElement->FirstChildElement();
				if(NULL==SpNumberElement)
				{
	                //std::cout << "SpNumberElement is NULL." << std::endl;
	                return false;
	            }

	            TiXmlElement *UserNumberElement = SpNumberElement->NextSiblingElement();
				if(NULL==UserNumberElement)
				{
	                //std::cout << "UserNumberElement is NULL." << std::endl;
	                return false;
	            }

				TiXmlElement *MoMsgElement = UserNumberElement->NextSiblingElement();
				if(NULL==MoMsgElement)
				{
	                //std::cout << "MoMsgElement is NULL." << std::endl;
	                return false;
	            }

				TiXmlElement *MoTimeElement = MoMsgElement->NextSiblingElement();
				if(NULL==MoTimeElement)
				{
	                //std::cout << "MoTimeElement is NULL." << std::endl;
	                return false;
	            }

				TiXmlElement *timestampElement = MoTimeElement->NextSiblingElement();
				if(NULL==timestampElement)
				{
	                //std::cout << "timestampElement is NULL." << std::endl;
	                return false;
	            }

				TiXmlNode* SpNumberNode = SpNumberElement->FirstChild();
				TiXmlNode* UserNumberNode = UserNumberElement->FirstChild();
				TiXmlNode* MoMsgNode = MoMsgElement->FirstChild();
				TiXmlNode* MoTimeNode = MoTimeElement->FirstChild();
				TiXmlNode* timestampNode = timestampElement->FirstChild();

				smsp::UpstreamSms upstream;
				
				if(NULL !=SpNumberNode && NULL!= UserNumberNode && NULL != MoMsgNode && NULL != MoTimeNode && NULL != timestampNode)
				{
					std::string strBaseNumber = BaseNumberNode->Value();
					std::string strSpNumber = SpNumberNode->Value();

					upstream._phone = UserNumberNode->Value();
					upstream._appendid = strSpNumber.substr(strBaseNumber.length());
					upstream._upsmsTimeStr = MoTimeNode->Value();
					upstream._content = MoMsgNode->Value();

					upsmsRes.push_back(upstream);
	                
		        }
				else
				{
					//std::cout << "report data error" << std::endl;
		            return false;
				}
					
				responseDataElement = responseDataElement->NextSiblingElement();//下一个节点
			}
		}
		else
		{
			//std::cout << "type error: " << type<< std::endl;
			return false;
		}
        
        strResponse = "<?xml version=\"1.0\" encoding=\"utf-8\"?><root><RetCode>0</RetCode><RetMsg>success</RetMsg></root>";
        return true;
	}
	catch(...)
	{
		//std::cout << "[CXDYChannellib::parseReport is failed." << std::endl;
		return false;
	}
}


int CXDYChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
{	
	time_t iCurtime = time(NULL);
	char chTime[64] = { 0 };
	struct tm  tmp = {0};
	localtime_r((time_t*) &iCurtime,&tmp);
	strftime(chTime, sizeof(chTime), "%Y-%m-%d %H:%M:%S", &tmp);
	////LogNotice("date[%s]", chTime);
	

	/*
	<?xml version="1.0" encoding="utf-8"?>
	<sendmail>
		<request_method>sendmail</request_method>
		<app_key>55619c5a-eaaa-4f55-bbd5-742c59e5a05e</app_key>
		<app_secret>db141c4c-687d-442b-b35f-09c53d78ea2d</app_secret>
		<sign_code>%CXDYMD5%</sign_code>
		<timestamp>%CXDYTIME%</timestamp>
		<version>2.0</version>
		<receiver_mail>%phone%@139.com</receiver_mail>
		<receiver_name></receiver_name>
		<content></content>
		<title>%content%</title>
		<username></username>
		<return_format>xml</return_format>
		<usernumber>yunzhixun_sms</usernumber>
		<sendsmspriority>2</sendsmspriority>
		<spnumber>10658139990008%displaynum%</spnumber>
		<templateId></templateId>
	</sendmail>
	*/
	
    string strSrc = "";
    std::string::size_type pos = data->find("<app_key>");
    std::string::size_type pos1 = data->find("</app_key>");
	strSrc.append("app_key");
    strSrc.append(data->data(),pos+strlen("<app_key>"),pos1-pos-strlen("<app_key>"));
    
    pos = data->find("<app_secret>");
    pos1 = data->find("</app_secret>");
	strSrc.append("app_secret");
    strSrc.append(data->data(),pos+strlen("<app_secret>"),pos1-pos-strlen("<app_secret>"));

    strSrc.append("contentreceiver_mail%CXDYPHONE%@139.comreceiver_namerequest_methodsendmailreturn_formatxmlsendsmspriority2spnumber%CXDYUSERNUMBER%templateIdtimestamp%CXDYTIME%title%CXDYCONTENT%usernameusernumberyunzhixun_smsversion2.0");
    
	string strMD5src = strSrc;
	
	pos = strMD5src.find("%CXDYPHONE%");
	if(pos != std::string::npos)
	{
		strMD5src.replace(pos, strlen("%CXDYPHONE%"), sms->m_strPhone);
	}

	pos = data->find("<spnumber>");
    pos1 = data->find("%displaynum%</spnumber>");
	std::string str_spnumber;
	str_spnumber.assign(data->data(),pos+strlen("<spnumber>"),pos1-pos-strlen("<spnumber>")).append(sms->m_strShowPhone);
	
	pos = std::string::npos;
	pos = strMD5src.find("%CXDYUSERNUMBER%");
	if(pos != std::string::npos)
	{
		strMD5src.replace(pos, strlen("%CXDYUSERNUMBER%"), str_spnumber);
	}

	pos = std::string::npos;
	pos = strMD5src.find("%CXDYTIME%");	//2015-12-14 21:23:07
	if(pos != std::string::npos)
	{	
		
		
		strMD5src.replace(pos, strlen("%CXDYTIME%"), chTime);	//time
	} 

	pos = std::string::npos;
	pos = strMD5src.find("%CXDYCONTENT%");
	if(pos != std::string::npos)
	{	
		strMD5src.replace(pos, strlen("%CXDYCONTENT%"), smsp::Channellib::encodeBase64(sms->m_strContent));	
	}

	//get md5
	unsigned char md5[16] = {0};
	MD5((const unsigned char*) strMD5src.data(), strMD5src.length(), md5);
	std::string strMD5 = "";
	std::string HEX_CHARS = "0123456789ABCDEF";
	for (int i = 0; i < 16; i++)
	{
		strMD5.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
		strMD5.append(1, HEX_CHARS.at(md5[i] & 0x0F));
	}

	//std::cout << "strMD5src: " << strMD5src << std::endl;
	//std::cout << "strMD5: " << strMD5 << std::endl;


	pos = std::string::npos;
	pos = data->find("%content%");
	if (pos != std::string::npos) 
	{
		data->replace(pos, strlen("%content%"),smsp::Channellib::encodeBase64(sms->m_strContent));		
	}

	pos = std::string::npos;
	pos = data->find("%phone%");
	if (pos != std::string::npos) 
	{
		data->replace(pos, strlen("%phone%"), sms->m_strPhone);		
	}

	pos = std::string::npos;
	pos = data->find("%CXDYMD5%");
	if (pos != std::string::npos) 
	{
		data->replace(pos, strlen("%CXDYMD5%"), strMD5);		
	}

	pos = std::string::npos;
	pos = data->find("%CXDYTIME%");
	if (pos != std::string::npos) 
	{
		data->replace(pos, strlen("%CXDYTIME%"), chTime);		
	}
	
	return 0;
}

int CXDYChannellib::getChannelInterval(SmsChlInterval& smsChIntvl)		//永远隔60s取一次。
{
	smsChIntvl._itlValLev1 = 20;	//val for lev2, num of msg
	//smsChIntvl->_itlValLev2 = 20;	//val for lev2

	smsChIntvl._itlTimeLev1 = 60*10;		//time for lev1, time of interval
	smsChIntvl._itlTimeLev2 = 60*10;		//time for lev 2
	//smsChIntvl->_itlTimeLev3 = 1;		//time for lev 3
	
}


} /* namespace smsp */
