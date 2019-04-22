#include "WLWXChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include "xml.h"

namespace smsp
{

    WLWXChannellib::WLWXChannellib()
    {
    }
    WLWXChannellib::~WLWXChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new WLWXChannellib;
        }
    }

    void WLWXChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool WLWXChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        //content[SUCCESS:%E6%8F%90%E4%BA%A4%E6%88%90%E5%8A%9F%EF%BC%81
        //13632697284:59106108111446017263:0
        //]
        //ps: content[] are not from WLWX
        try
        {
            printf("%s\n", respone.data());
            //LogNotice("wlwx parseSend! response[%s]",respone.data());
            std::vector<char*> list;
            char szResult[1024] = { 0 };

            std::string rpt;
            std::string::size_type pos = respone.find_first_of('\n');
            if (pos != std::string::npos)
            {
                rpt = respone.substr(pos + 1);
            }
            else
            {
                //LogNotice("WLWX report find no huiche");
                return false;
            }

            sprintf(szResult, "%s", rpt.data());
            split(szResult, ":", list);

            if(list.size()<3)
            {
                return false;
            }


            //get status
            std::string rst = list.at(2);
            rst = rst.substr(0, rst.length()-2);        // delete /r/n in the end
            if(rst == "0")  // 0:成功， 15:通道不支持号码
            {
                status = "0";
            }
            else
            {
                status = rst;
                //LogNotice("WLWX report status not 0,response[%s]", respone.data());
                return false;
            }

            //get desc
            strReason = rst;    //通道回复回来的错误码，放入应答描述中

            //get smsid
            std::string msgid = list.at(1);
            if(!msgid.empty())
            {
                smsid = msgid;
            }
            else
            {
                //LogNotice("WLWX report smsid is null,response[%s]", respone.data());
                return false;
            }


            //smsid = list.at(1);
            return true;
        }
        catch(...)
        {
            //LogNotice("[WLWXChannellib::parseSend] is failed.")
            return false;
        }
    }

    bool WLWXChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        //LogNotice("fjx: parseReport! response[%s]",respone.data());
        //report=59106108111620019677,13632697284,DELIVRD,2015-08-11 16:20:33^CoA
        try
        {
            strResponse.assign("ok");
            /* BEGIN: Added by fuxunhao, 2015/8/28   PN:smsp_SSDJ */
            if (0 == respone.compare(0, strlen("msg_id"), "msg_id"))
            {
                smsp::UpstreamSms upsms;
                std::vector<char*> upList;

                split((char*)respone.data(), "&", upList);
                if (upList.size() < 5)
                {
                    //LogError("[WLWXParser::parseReport] parse upsms is failed.");
                    return false;
                }

                std::string strSp_code  = upList.at(1);
                std::string strSrc_mobile = upList.at(2);
                std::string strMsg_content = upList.at(3);
                std::string strRecv_time = upList.at(4);

                strRecv_time = strRecv_time.substr(0, strRecv_time.length()-2);

                strSp_code = strSp_code.substr(strlen("sp_code="));
                strSrc_mobile = strSrc_mobile.substr(strlen("src_mobile="));
                strMsg_content = strMsg_content.substr(strlen("msg_content="));
                strRecv_time= strRecv_time.substr(strlen("recv_time="));

                upsms._appendid = strSp_code;/////.substr(strSp_code.length() - 3);
                upsms._phone = strSrc_mobile;
                upsms._upsmsTimeStr = strRecv_time;
                upsms._upsmsTimeInt = strToTime((char*)strRecv_time.data());
                ///upsms._content = strMsg_content;
                upsms._content = smsp::Channellib::urlDecode(strMsg_content);

                //LogNotice("strSp_code[%s],strSrc_mobile[%s],strMsg_content[%s],strRecv_time[%s].",
                //          strSp_code.data(), strSrc_mobile.data(), upsms._content.data(), strRecv_time.data());

                upsmsRes.push_back(upsms);

                return true;
            }
            /* END:   Added by fuxunhao, 2015/8/28   PN:smsp_SSDJ */

            smsp::StateReport report;
            int nStatus;
            std::vector<char*> reportslist;
            std::vector<char*> infolist;
            std::vector<char*> infodata;
            char* szResult = new char[respone.length()+1];
            memset(szResult, 0x0, respone.length()+1);

            int nPos;
            if (std::string::npos != (nPos = respone.find("=")))
                respone = respone.substr(nPos + 1, respone.length());

            sprintf(szResult, "%s", respone.data());
            split(szResult, "^", reportslist);
            size_t reportsize = reportslist.size();

            if(reportsize==0)
            {
                //LogNotice("WLWX report size[%d]", reportsize);
                delete []szResult;
                return false;
            }

            for(int i=0; i<reportsize; i++)
            {
                sprintf(szResult, "%s", reportslist.at(i));
                infolist.clear();
                split(szResult, ",", infolist);
                if(infolist.size()<4)
                    continue;

                std::string str0 = infolist.at(0);  //      59106108111620019677
                std::string str1 = infolist.at(1);  //      13632697284
                std::string str2 = infolist.at(2);  //      DELIVRD
                std::string str3 = infolist.at(3);  //      2015-08-11 16:20:33

                //LogNotice("fjx: str0:[%s],str1:[%s],str2:[%s],str3:[%s]", str0.data(), str1.data(), str2.data(), str3.data());
                //3th word:status
                if (!strcmp(str2.data(), "DELIVRD"))    //为 DELIVRD
                    nStatus = SMS_STATUS_CONFIRM_SUCCESS;
                else
                {
                    nStatus = SMS_STATUS_CONFIRM_FAIL;
                    //LogNotice("WLWX report status not DELIVERD, szResult[%S]", szResult);
                }

                report._smsId = str0;
                report._status = nStatus;
                char reptime[100] = {0};
                sprintf(reptime, "%s", str3.data());
                report._reportTime = strToTime(reptime);
                report._desc = str2;
                reportRes.push_back(report);
            }

            delete []szResult;
            return true;
        }
        catch(...)
        {
            //LogNotice("[WLWXChannellib::parseReport] is failed.")
            return false;
        }
    }


    int WLWXChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
        //REPLACE %sign%
        int pos = data->find("%sign%");
        std::string strSign = "";
        //web::UrlCode::UrlEncode(sendContent)
        strSign.append(smsp::Channellib::urlEncode(sms->m_strContent));
        strSign.append("3WO1MYBO92");   //WLWX pwd for 530056

        unsigned char md5[16] = {0};
        MD5((const unsigned char*) strSign.data(), strSign.length(), md5);

        std::string strMD5 = "";
        std::string HEX_CHARS = "0123456789ABCDEF";
        for (int i = 0; i < 16; i++)
        {
            strMD5.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
            strMD5.append(1, HEX_CHARS.at(md5[i] & 0x0F));
        }

        data->replace(pos, strlen("%sign%"), strMD5);
        return 0;
    }

} /* namespace smsp */
