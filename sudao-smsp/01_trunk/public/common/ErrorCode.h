#ifndef __ERROR_CODE__
#define __ERROR_CODE__

#include "platform.h"

/// smsp error code

const std::string ERROR_CODE_SMSP_OVER_RATE                    = "500";
const std::string ERROR_CODE_SMSP_NO_CHANNEL                   = "501";
const std::string ERROR_CODE_SMSP_INCLUDE_HTTP                 = "502";
const std::string ERROR_CODE_SMSP_CONTENT_EMPITY               = "503";
const std::string ERROR_CODE_SMSP_PHONE_ERROR                  = "504";
const std::string ERROR_CODE_SMSP_SUMBIT_CHANNEL_FAIL          = "505";
const std::string ERROR_CODE_SMSP_BLACK_LIST                   = "506";
const std::string ERROR_CODE_SMSP_KEY_LIST                     = "507";
const std::string ERROR_CODE_SMSP_NO_STATUS_REPORT             = "508";
const std::string ERROR_CODE_SMSP_INTERNAL_ERROR               = "509";
const std::string ERROR_CODE_SMSP_SESSIONID_REPEATED           = "510";
const std::string ERROR_CODE_SMSP_TIMEOUT              		   = "511";
const std::string ERROR_CODE_SMSP_HOSTIPERROR             	   = "512";


/// channel error code
const std::string ERROR_CODE_SIGN_REFUSE                        = "600";
const std::string ERROR_CODE_EXPIRED                            = "601";
const std::string ERROR_CODE_REJECTD                            = "602";
const std::string ERROR_CODE_UNDELIV                            = "603";
const std::string ERROR_CODE_NOROUTE                            = "604";
const std::string ERROR_CODE_OTHER_CHANNEL_ERROR                = "605";
const std::string ERROR_CODE_BLACK_LIST                         = "606";
const std::string ERROR_CODE_KEY_LIST                           = "607";
const std::string ERROR_CODE_OVER_RATE                          = "608";
const std::string ERROR_CODE_CONTENT_REFUSE                     = "609";

/// sms db status
const UInt32 SMS_STATUS_INITIAL                 = 0;
const UInt32 SMS_STATUS_SUBMIT_SUCCESS          = 1;
const UInt32 SMS_STATUS_SEND_SUCCESS            = 2;
const UInt32 SMS_STATUS_CONFIRM_SUCCESS         = 3;
const UInt32 SMS_STATUS_SUBMIT_FAIL             = 4;
const UInt32 SMS_STATUS_SEND_FAIL               = 5;
const UInt32 SMS_STATUS_CONFIRM_FAIL            = 6;
const UInt32 SMS_STATUS_AUDIT_FAIL              = 7;
const UInt32 SMS_STATUS_GW_RECEIVE_HOLD         = 8;
const UInt32 SMS_STATUS_GW_SEND_HOLD            = 9;
const UInt32 SMS_STATUS_OVERRATE                = 10;
const UInt32 SMS_STATUS_HUMAN_SUBMIT_SUCCESS    = 11;
const UInt32 SMS_STATUS_HUMAN_CONFIRM_SUCCESS   = 13;

const UInt32 SMS_STATUS_SOCKET_ERR_TO_REBACK    = 14;




// sms db audit status
const unsigned int SMS_AUDIT_STATUS_WAIT                           = 0;
const unsigned int SMS_AUDIT_STATUS_PASS                           = 1;
const unsigned int SMS_AUDIT_STATUS_FAIL                           = 2;
const unsigned int SMS_AUDIT_STATUS_AUTO_PASS                      = 3;
const unsigned int SMS_AUDIT_STATUS_AUTO_FAIL                      = 4;
const unsigned int SMS_AUDIT_STATUS_CACHE                          = 5;
const unsigned int SMS_AUDIT_STATUS_BLACK_TEMP                     = 6;
const unsigned int SMS_AUDIT_STATUS_TIMEOUT                        = 7;        // 审核超时
const unsigned int SMS_AUDIT_STATUS_GROUPSENDLIMIT                 = 8;        // 审核超时
const unsigned int SMS_AUDIT_STATUS_ERROR                          = 1000;

//YZX nei bu cuo wu ma
//need to tou chuan report
#define SEND_TO_SMSP_SEND_FAILED				"YX:4000"//send to smsp_send failed
#define SEND_TO_SMSP_SEND_TIMEOUT				"YX:4001"//send to smsp_send timeout
#define HOSTIP_CHARGE_IP_FAILED					"YX:5001"//hostIp charge ip failed
#define HTTPSENDER_INIT_FAILED					"YX:5002"//HttpSender init is failed
#define SMSP_AUDIT_RETURN_ERROR					"YX:5004"//smsp_audit return error
#define CHARGE_FAILED							"YX:5006"//charge fail
#define CHECK_AUDIT_REDIS_TIMEOUT				"YX:5007"//check audit redis timeout
#define SEND_TO_AUDIT_TIMEOUT					"YX:5008"//send to audit timeout
#define CHARGE_TIMEOUT							"YX:5009"//charge timeout
#define AUDIT_NOT_PAAS							"YX:7000"//audit not paas
#define CLIENT_BLACK_TEMPLATE					"YX:7001"//match client black template
#define GLOBAL_BLACK_TEMPLATE					"YX:7002"//match global black template
#define AUDIT_TIMEOUT        					"YX:7003"//audit timeout
#define MQ_LINK_ERROR_AND_QUEUESIZE_TOO_LARGE 	"YX:8010"//MQ link error and Queuesize too large
#define HOSTIP_ANALYZE_FAILED					"YX:9000"//hostIp analyze failed
#define PHONE_SEGMENT_ERROR						"YX:9001"//phone segment error
#define INTERNAL_ERROR							"YX:9002"//internal error
#define NO_SMPP_SALE_PRICE						"YX:9003"//no smpp sale price
#define NO_CHANNEL_GROUPS						"YX:9004"//no channel Groups
#define OVER_SELECT_CHANNEL_MAX_NUM				"YX:9005"//over select channel max num
#define NO_CHANNEL								"YX:9006"//no channel
#define NO_SMPP_PRICE							"YX:9007"//no smpp price
#define CHARGE_IS_FAILED						"YX:9008"//charge is failed
#define SMSP_SEND_NODE_NOT_IN_SEND_NODE_TABLE	"YX:9010"//smsp_send node not in sendNodeTable
#define HOSTIP_SEND_IP_FAILED					"YX:9011"//hostIp send ip failed
#define OVERRATE_OR_HOSTIP_TIMEOUT				"YX:9012"//overrate or hostip timeout
#define INVALID_TIMEOUT							"YX:9013"//invalid timeout
#define HTTPSEND_INIT_FAILED					"YX:9014"//httpSend init failed
#define OVER_RATE								"YX:1000"//over Rate
#define KEYWORD_OVER_RATE						"YX:1006"//key word over rate
#define GLOBAL_OVER_RATE						"YX:1007"//global over rate
#define CHANNEL_OVER_RATE						"YX:1008"//channel over rate

#define SOCKET_IS_ERROR							"YX:4010"//socket is error
#define SOCKET_IS_CLOSE							"YX:4011"//socket is close
#define RESPONSE_IS_NOT_200						"YX:4012"//response is not 200
#define T_SMS_CHANNEL_ERROR_DESC				"YX:4013"
#define LIB_IS_NULL								"YX:4014"//lib is null
#define REBACK_OVER_TIME                        "YX:4002"  //abnormal messages over remaining time

//need to update db
//#define SEND_TO_SMSP_SEND_OK					"YX:4002"//send to smsp_send ok
#define OVER_RATE_HTTPSENDER_INIT_IS_FAILED		"YX:5003"//over Rate|HttpSender init is failed
#define OVER_RATE_CHARGE_FAIL_1					"YX:5005"//over Rate|charge fail(state = 5)
#define OVER_RATE_CHARGE_TIMEOUT				"YX:5010"//over Rate|charge timeout
#define CLIENTID_IS_INVALID						"YX:8000"//clientid is invalid
#define CHINESE_1								"YX:8001"//缺少请求参数或参数不正确//6
#define CHINESE_2								"YX:8002"//短信内容超长或签名超长//8
#define CHINESE_3								"YX:8003"//账号不存在//1
#define CHINESE_4								"YX:8004"//账号被锁定//4
#define CHINESE_5								"YX:8005"//ip鉴权失败//5
#define SMSFROM_IS_NOT_MATCH					"YX:8006"//smsfrom is not match
#define PHONE_FORMAT_IS_ERROR					"YX:8007"//phone format error
#define BLACKLIST								"YX:8008"//blacklist
#define REPEAT_PHONE							"YX:8009"//repeat phone
#define ACCOUNT_LOCKED							"YX:8011"//account locked
#define CONTENT_LENGTH_TOO_LONG					"YX:8012"//content length too long
#define PHONE_COUNT_TOO_MANY					"YX:8013"//phone count too many
#define PKTOTAL_TOO_MANY						"YX:8014"//pktotal too many
#define PKNUMBER_BIG_PKTOTAL					"YX:8015"//pkNumber big pkTotal
#define CAN_NOT_GET_ALL_SHORT_MSGS				"YX:8016"//can not get all short msgs.
#define LONG_SMS_MERGE_TIMEOUT                  "YX:8017"  //long sms merge timeout
#define CONTENT_EMPTY                           "YX:8018"  //content empty
#define BLACKLIST_KEYWORD                       "YX:8019"  //keyword
#define SIGN_NOT_REGISTER						"YX:8023"  ///clientid and sign not register in table t_sms_clientid_signport
#define OVER_RATE_CHARGE_FAIL_2					"YX:9009"//over Rate|charge fail(state = 9)
#define TIME_OUT								"YX:1001"//time out(http cnannel reponse time out)
#define CMPP_REPLY_TIME_OUT						"YX:1002"//cmpp reply time out
#define SGIP_REPLY_TIME_OUT						"YX:1003"//sgip reply time out
#define SMGP_REPLY_TIME_OUT						"YX:1004"//smgp reply time out
#define SMPP_REPLY_TIME_OUT						"YX:1005"//smpp reply time out
#define CMPP_INTERNAL_ERROR						"YX:4015"//cmpp internal error
#define SMPP_CONNECTION_ABNORMAL				"YX:4016"//smpp connection abnormal
#define SGIP_INTERNAL_ERROR						"YX:4017"//sgip internal error
#define SMGP_INTERNAL_ERROR						"YX:4018"//smgp internal error
#define SMPP_INTERNAL_ERROR						"YX:4019"//smpp internal error
#define SEND_AUDIT_INTERCEPT					"YX:4020"//send audit intercept

#define C2S_MQ_CONFIG_ERROR						"YX:9015"

#define ERRORCODE_TEMPLATEID_NOEXIST            "YX:8021"    //用户对应的模板ID不存在、或模板未通过审核、或模板已删除
#define ERRORCODE_TEMPLATE_PARAM_ERROR          "YX:8022"    //模板参数错误
#define ERRORCODE_TEMPLATEID_IS_NULL            "YX:8024"    //模板ID为空
#define ERRORCODE_TEMPLATE_PHONE_NO_MATCH       "YX:8025"    //USSD、闪信和挂机短信模板不允许发送国际号码
#define ERRORCODE_CONTENT_ENCODE_ERROR			"YX:8026"

#define USER_BLACKLIST							"YX:8027"//blacklist
#define BLANK_PHONE_ERROR						"YX:8028"//blankphone
#define TCP_FLOW_CTRL						    "YX:8029"//blankphone


#endif
