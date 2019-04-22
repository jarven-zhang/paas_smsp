#include "ITRIGOChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "xml.h"

namespace smsp
{

    ITRIGOChannellib::ITRIGOChannellib()
    {

    }
    ITRIGOChannellib::~ITRIGOChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new ITRIGOChannellib;
        }
    }

    bool ITRIGOChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        try
        {
            std::string::size_type pos = respone.find("0");
            if (pos != std::string::npos)
            {
                status = "0";
            }
            else
            {
                status = "1";
                strReason = respone;
            }
            return true;

        }
        catch(...)
        {
            //LogError("[ITRIGOChannellib::parseSend] is failed.");
            return false;
        }
    }

    bool ITRIGOChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        ////100{&}50657010227||13512341234||DELIVRD||20130705103005{&}657010227||13512341234||DELIVRD||20130705103005
        try
        {
            if (respone.length() < 17)
            {
                //LogNotice("[ITRIGOChannellib] respone is short");
                return false;
            }

            std::string strContent = respone;
            std::string strStatus = strContent.substr(0,3);
            if (0 != strStatus.compare("100"))
            {
                //LogError("[ITRIGOChannellib] strStatus[%s] verify is failed.", strStatus.data());
                return false;
            }

            strContent = strContent.substr(6);

            std::vector<char*> vecReports;
            split((char*)strContent.data(), "{", vecReports);

            int iSize = vecReports.size();

            if (0 == iSize)
            {
                //LogError("[ITRIGOChannellib] vecReports size is 0");
                return false;
            }

            for (int i = 0; i < iSize; i++)
            {
                std::vector<char*>vecReport;
                char* pTmp = NULL;

                if (0 == i)
                {
                    pTmp = vecReports.at(i);
                }
                else
                {
                    pTmp = vecReports.at(i);
                    pTmp = pTmp + 2;
                }

                split(pTmp, "||", vecReport);

                if (7 != vecReport.size())
                {
                    //LogError("[ITRIGOChannellib] size:%d is error.", vecReport.size())
                    continue;
                }

                smsp::StateReport report;

                std::string strMsgid = vecReport.at(0);
                std::string strPhone = vecReport.at(2);
                std::string strDesc = vecReport.at(4);
                std::string strTime = vecReport.at(6);

                //LogNotice("strMsgid[%s], strPhone[%s], strDesc[%s], strTime[%s].", strMsgid.data(),strPhone.data(),strDesc.data(),strTime.data());
                if (0 != strDesc.compare("DELIVRD"))
                {
                    report._status = SMS_STATUS_CONFIRM_FAIL;
                }
                else
                {
                    report._status = SMS_STATUS_CONFIRM_SUCCESS;
                }

                report._desc = strDesc;
                report._smsId = strMsgid;
                report._reportTime = strToTime(vecReport.at(6));

                reportRes.push_back(report);

            }


            return true;
        }
        catch(...)
        {
            //LogError("[ITRIGOChannellib::parseReport] is failed.");
            return false;
        }
    }


    int ITRIGOChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
        if(sms->m_strChannelSmsId.length() > 20)
        {
            std::string strMsgid = sms->m_strChannelSmsId.substr(0,20);
            sms->m_strChannelSmsId = strMsgid;

            std::string::size_type pos = data->find("%msgid%");
            if (pos != std::string::npos)
            {
                data->replace(pos, strlen("%msgid%"), strMsgid);
            }
        }

        return 0;
    }


} /* namespace smsp */
