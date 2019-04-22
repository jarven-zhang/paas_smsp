#include "YXDYChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include "xml.h"

namespace smsp 
{

YXDYChannellib::YXDYChannellib() 
{
}
YXDYChannellib::~YXDYChannellib() 
{

}

extern "C"
{
    void * create()
    {
        return new YXDYChannellib;
    }
}

bool YXDYChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
{	

	/*
	<?xml version="1.0" encoding="utf-8"?> 
	<responsedata> 
	<resultcode>000</resultcode> 
	<resultdesc>操作成功</resultdesc> 
	<return> 
	<subretcode>0</subretcode> 
	<smssendcode>0</smssendcode> 
	<smsid>5412854568565869</smsid>
	<timestamp >1476436722742</timestamp>
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
			return false;
		}

		TiXmlElement *element = myDocument.RootElement();
		if(NULL==element)
		{
			return false;
		}

		////resultcode
		TiXmlElement *resultcodeElement = element->FirstChildElement();
		if(NULL==resultcodeElement)
		{	
			return false;
		}
		TiXmlNode *resultcodeNode = resultcodeElement->FirstChild();
		if(NULL==resultcodeNode)
		{
            return false;
        }
		
		////resultdesc
		TiXmlElement *resultdescElement = resultcodeElement->NextSiblingElement();
		if(NULL==resultdescElement)
		{	
			return false;
		}
		TiXmlNode *resultdescNode = resultdescElement->FirstChild();
		if(NULL==resultdescNode)
		{
            ///return false;
        }
		
		std::string str_resultcode = resultcodeNode->Value();
		std::string str_resultdesc = resultdescNode->Value();

		////return
		TiXmlElement *returnElement = resultdescElement->NextSiblingElement();
		if(NULL==returnElement)
		{	
			status = "4";
			strReason = str_resultdesc;	
			return true;
		}

		////subretcode
		TiXmlElement *subretcodeElement = returnElement->FirstChildElement();
		if(NULL==subretcodeElement)
		{
            return false;
        }
		TiXmlNode *subretcodeNode = subretcodeElement->FirstChild();
		if(NULL==subretcodeNode)
		{
            return false;
        }

		////smssendcode
		TiXmlElement *smssendcodeElement = subretcodeElement->NextSiblingElement();
		if(NULL==smssendcodeElement)
		{
            return false;
        }
		TiXmlNode *smssendcodeNode = smssendcodeElement->FirstChild();
		if(NULL==smssendcodeNode)
		{
            return false;
        }

		std::string strSmsSendCode = smssendcodeNode->Value();

		////smsid
		TiXmlElement *smsidElement = smssendcodeElement->NextSiblingElement();
		if(NULL==smsidElement)
		{
            return false;
        }
		TiXmlNode *smsidNode = smsidElement->FirstChild();
		if(NULL==smsidNode)
		{
            return false;
        }
		
		std::string str_smsid = smsidNode->Value();
		
		if(strSmsSendCode == "0")
		{
			status = "0";
			smsid = str_smsid;
			strReason = strSmsSendCode;
		}
		else
		{
			status = "4";
			
		}
		smsid = str_smsid;
		strReason = strSmsSendCode;
		
		return true;
	}
	catch(...)
	{
		return false;
	}
}

bool YXDYChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse) 
{	
/*
////report
<?xml version="1.0" encoding="utf-8"?>
<root>
<seqno>2771156304</seqno>
<smsnum>3</smsnum>

<responseData> 
<MsgId>2771156304</MsgId>  
<SmsMsg>DELIVRD</SmsMsg>  
<SendStatus>0</SendStatus>  
<ReceiveNum>13512345678</ReceiveNum>
<Timestamp>1476436722742</Timestamp>
<ReceiveTime>20150522 10:28:41</ReceiveTime> 
</responseData> 

<responseData> 
<MsgId>2771156305</MsgId>  
<SmsMsg> DELIVRD </SmsMsg>    
<SendStatus>0</SendStatus>  
<ReceiveNum>13512345688</ReceiveNum>
<Timestamp>1476436722742</Timestamp>
<ReceiveTime>20150522 10:28:41</ReceiveTime>
</responseData> 
</root>

////mo
<root>
<responseData>
<Seqno>XXX</Seqno>
<SpNumber>XXX</SpNumber>
<UserNumber>XXX</UserNumber>
<MoMsg>XXX</MoMsg>
<MoTime>2016-02-28 16:52:47</MoTime>
<timestamp>XXX</timestamp>
</responseData>

<responseData>
<Seqno>XXX</Seqno>
<SpNumber>XXX</SpNumber>
<UserNumber>XXX</UserNumber>
<MoMsg>XXX</MoMsg>
<MoTime>2016-02-28 16:52:48</MoTime>
<timestamp>XXX</timestamp>
</responseData>
</root>
*/
	try
	{
		std::string::size_type sizePos = respone.find_first_of("<");
		if (std::string::npos == sizePos)
		{
			return false;
		}
		respone = respone.substr(sizePos);

		sizePos = respone.find_last_of(">");
		if (std::string::npos == sizePos)
		{
			return false;
		}
		respone.erase(sizePos+1);
	
		TiXmlDocument myDocument;

		if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
		{
			return false;
		}

		TiXmlElement *elementRoot = myDocument.RootElement();
		if(NULL==elementRoot)
        {
			return false;
		}

		
		TiXmlElement* elementFirst = elementRoot->FirstChildElement();
		if (NULL == elementFirst)
		{
			return false;
		}
		
		string strFirst = elementFirst->Value();
	
		if ("seqno" == strFirst)   ////report
		{	
			////smsnum
	        TiXmlElement* pSmsNum = elementFirst->NextSiblingElement();
	        if(NULL==pSmsNum)
	        {
				return false;
			}

			////responseData
			TiXmlElement *responseDataElement = pSmsNum->NextSiblingElement();
			if(NULL==responseDataElement)
			{	
				return false;
			}

			while(responseDataElement != NULL)
			{	
				////MsgId
	            TiXmlElement *MsgIdElement = responseDataElement->FirstChildElement();
				if(NULL==MsgIdElement)
				{
	                return false;
	            }

				////SmsMsg
				TiXmlElement *SmsMsgElement = MsgIdElement->NextSiblingElement();
				if(NULL==SmsMsgElement)
				{
	                return false;
	            }
			
				////SendStatus
				TiXmlElement *SendStatusElement = SmsMsgElement->NextSiblingElement();
				if(NULL==SendStatusElement)
				{
	                return false;
	            }

				////ReceiveNum
				TiXmlElement* ReceiveNumElement = SendStatusElement->NextSiblingElement();
				if (NULL == ReceiveNumElement)
				{
					return false;
				}

				////Timestamp
				TiXmlElement* TimestampElement = ReceiveNumElement->NextSiblingElement();
				if (NULL == TimestampElement)
				{
					return false;
				}
			
				////ReceiveTime
				TiXmlElement *ReceiveTimeElement = TimestampElement->NextSiblingElement();
				if(NULL==ReceiveTimeElement)
				{
	                return false;
	            }

				std::string ReceiveTime;
				std::string SmsMsg;
				std::string MsgId;
				std::string SendStatus;

				TiXmlNode *ReceiveTimeNode = ReceiveTimeElement->FirstChild();
				TiXmlNode *SmsMsgNode = SmsMsgElement->FirstChild();
				TiXmlNode *MsgIdNode = MsgIdElement->FirstChild();
				TiXmlNode *SendStatusNode = SendStatusElement->FirstChild();
				smsp::StateReport report;
				
				if((NULL !=ReceiveTimeNode)
					&& (NULL!= SmsMsgNode)
					&& (NULL != MsgIdNode)
					&& (NULL != SendStatusNode))
				{
		           	report._reportTime = time(NULL);
					report._desc  = SmsMsgNode->Value();
					report._smsId = MsgIdNode->Value();
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
		            return false;
				}
					
				responseDataElement = responseDataElement->NextSiblingElement();//下一个节点
			}
			
		}
		else if ("responseData" == strFirst) ////mo  2017-03-06 mo is not support!
		{
			while(elementFirst != NULL)
			{
				////Seqno
		        TiXmlElement* pSeqNo = elementFirst->FirstChildElement();
		        if(NULL==pSeqNo)
		        {
					return false;
				}

				////SpNumber
				TiXmlElement* pSpNumber = pSeqNo->NextSiblingElement();
				if (NULL == pSpNumber)
				{
					return false;
				}

				////UserNumber
				TiXmlElement* pUserNumber = pSpNumber->NextSiblingElement();
				if (NULL == pUserNumber)
				{
					return false;
				}

				////MoMsg
				TiXmlElement* pMoMsg = pUserNumber->NextSiblingElement();
				if (NULL == pMoMsg)
				{
					return false;
				}

				////MoTime
				TiXmlElement* pMoTime = pMoMsg->NextSiblingElement();
				if (NULL == pMoTime)
				{
					return false;
				}

				////timestamp
				TiXmlElement* pTimeStamp = pMoTime->NextSiblingElement();
				if (NULL == pTimeStamp)
				{
					return false;
				}


				string strSpNumber = "";
				string strUserNumber = "";
				string strMoMsg = "";

				TiXmlNode* pSpNumberNode = pSpNumber->FirstChild();
				TiXmlNode* pUserNumberNode = pUserNumber->FirstChild();
				TiXmlNode* pMoMsgNode = pMoMsg->FirstChild();

				if((NULL !=pSpNumberNode) && (NULL!= pUserNumberNode) && (NULL != pMoMsgNode))
				{
					strSpNumber = pSpNumberNode->Value();
					strUserNumber = pUserNumberNode->Value();
					strMoMsg = pMoMsgNode->Value();

                  	if (0 == strUserNumber.compare(0,2,"86"))
                    {
                        strUserNumber = strUserNumber.substr(2);
                    }
				
                	time_t receiveTime = time(NULL);
                    struct tm pT = {0};
					localtime_r(&receiveTime,&pT);
                    char strT[50] = {0};
                    strftime(strT, 49, "%Y-%m-%d %X", &pT);

                    smsp::UpstreamSms upsms;
                    upsms._upsmsTimeStr = strT;
                    upsms._upsmsTimeInt = receiveTime;
                    upsms._phone = strUserNumber;
                    upsms._appendid = strSpNumber;
					upsms._content = strMoMsg;
					
					upsmsRes.push_back(upsms);
				}
	
				

				elementFirst = elementFirst->NextSiblingElement();//下一个节点
			}
		

		}
		else
		{
			return false;
		}

		return true;
	}
	catch(...)
	{
		return false;
	}
}


int YXDYChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
{
	return 0;
}

int YXDYChannellib::getChannelInterval(SmsChlInterval& smsChIntvl)
{
	smsChIntvl._itlValLev1 = 20;	//val for lev2, num of msg
	//smsChIntvl->_itlValLev2 = 20;	//val for lev2

	smsChIntvl._itlTimeLev1 = 30;		//time for lev1, time of interval
	smsChIntvl._itlTimeLev2 = 5;		//time for lev 2
	//smsChIntvl->_itlTimeLev3 = 1;		//time for lev 3
	
}


} /* namespace smsp */
