#include "SHSJChannellib.h"
#include <stdio.h>
#include <time.h>
#include "json/json.h"
#include <fstream>
#include "UTFString.h"
#include "xml.h"
#include "UTFString.h"
#include <map>
#include <stdlib.h>
#include "Channellib.h"

namespace smsp
{
    extern "C"
    {
        void * create()
        {
            return new SHSJChannellib;
        }
    }
    
    SHSJChannellib::SHSJChannellib()
    {
    }
    SHSJChannellib::~SHSJChannellib()
    {

    }

    void SHSJChannellib::test()
    {
    }

    bool SHSJChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
    	/*********************************************************************************************
    		respone:
		  			>0	�ɹ�,ϵͳ���ɵ�����id���Զ��������id
		  			�磺941219590864903168
    	**********************************************************************************************/
    	string_replace(respone, "\r\n", "");
		string_replace(respone, "\n", "");
		
		smsid  = respone;
		
		if(respone.find("-") == string::npos && respone != "0")
		{
			status = "0";	
			return true;
		}
		else
		{
			status = "-1";
			return false;
		}
    }

    bool SHSJChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
		/***************************************************************************************************************
			״̬����:
					report=����|״̬��|����ID|��չ��|����ʱ��;����|״̬��|����ID|��չ��|����ʱ��
				  �磺
					report=18638305275|DELIVRD|941216240207392912|13999|2017-12-14 16:00:18;18550205250|UNDELIV|941216283891075032|13999|2017-12-14 16:00:19

					״̬�룺
						DELIVRD ״̬�ɹ�
						UNDELIV ״̬ʧ��
						EXPIRED ��Ϊ�û���ʱ��ػ����߲��ڷ������ȵ��µĶ���Ϣ��ʱû�еݽ����û��ֻ���
						REJECTD ��Ϣ��ΪĳЩԭ�򱻾ܾ�
						MBBLACK �ں�

			���У�
					deliver=����|��չ��|�����ʽ|����|�û���|ʱ��

				  �磺
					deliver=T|13999|utf8|18638305275|yzx006|2017-12-14 16:00:18
		****************************************************************************************************************/
        std::vector<std::string> vectorAllReport;
		std::vector<std::string> vectorOneReport;

		if(respone.find("report=") == 0)
		{
			respone = respone.substr(strlen("report="));
		}
		else if(respone.find("deliver=") == 0)
		{
			respone = respone.substr(strlen("deliver="));
		}
		else
		{
			return false;
		}
		
		split_Ex_Vec(respone, ";", vectorAllReport);

		for(std::vector<std::string>::iterator iter = vectorAllReport.begin(); iter != vectorAllReport.end(); iter++)
		{
			vectorOneReport.clear();
			split_Ex_Vec(*iter, "|", vectorOneReport);

			if(vectorOneReport.size() == 5)//״̬����
			{
				smsp::StateReport report;
				
				report._smsId = vectorOneReport[2];
				report._phone = vectorOneReport[0];
				
				tm tm_time;
				strptime(vectorOneReport[4].data(), "%Y-%m-%d %H:%M:%S", &tm_time);
				report._reportTime = mktime(&tm_time); //��tmʱ��ת��Ϊ��ʱ��

				report._desc = 	vectorOneReport[1];
				
				if(report._desc == "DELIVRD")
				{
					report._status = SMS_STATUS_CONFIRM_SUCCESS;
				}
				else
				{
					report._status = SMS_STATUS_CONFIRM_FAIL;
				}

				reportRes.push_back(report);
			}
			else if(vectorOneReport.size() == 6)//����
			{
				smsp::UpstreamSms mo;
				
				mo._phone = vectorOneReport[3];
				mo._content = urlDecode(vectorOneReport[0]);
				mo._upsmsTimeStr = vectorOneReport[5];
				
				tm tm_time;
				strptime(vectorOneReport[2].data(), "%Y-%m-%d %H:%M:%S", &tm_time);
				mo._upsmsTimeInt = mktime(&tm_time); //��tmʱ��ת��Ϊ��ʱ��
				
				upsmsRes.push_back(mo);
			}
			else
			{
				;
			}
		}

		return true;
    }

    int SHSJChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
    	/*************************************************************************************************************
    	
    	data:
    		username=yzx081&password=yzx08163&mobile=%phone%&content=%content%
    		
		*************************************************************************************************************/		
    	std::map<string, string> mapData;
		split_Ex_Map(*data, "&", mapData);

		string_replace(*data, "&password=" + mapData["password"], "&password=" + CalMd5(mapData["username"] + CalMd5(mapData["password"])));

		vhead->push_back("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
		
        return 0;
	}
} /* namespace smsp */
