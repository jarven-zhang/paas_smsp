#ifndef _THREAD_MACRO_H_
#define _THREAD_MACRO_H_

/*
Naming Rules:
the prefix          :   'MSGTYPE'
the thread name :   eg:'ALARM' or 'DB' or 'HOSTIP'
the function name   :   eg: 'SURVIVE' or 'NOTQUERY' (optional)
the suffix          :   'REQ' or 'RESP'
the connector       :   '_'
'REQ'               :    even number
'RESP'              :    odd number
*/


/////////////////    msg time out           ////////////////
const int DISPATCH_SESSION_TIMEOUT          = 5 ;   //DSPH sessionmap timeout 5s

const int DISPATCH_QUERY_TIMEOUT            = 3 ;   //to redis and to host ip.  3s timeout

/////////////////    msg time out end     ////////////////


//===================    parma define    ===================//
const int TABLE_NUM = 10; 	// t_sms_access_n_2016****,   n:0,1,2,3... TABLE_NUM-1


//===================    parma define end   ===================//


//===================    msg error code define    ===================//
enum RESULTSTATUS
{
    SUCCEED = 0,
    FAILURE
};

//===================    msg error code define    ===================//

//===================    alarm type    ===================//

// 系统告警,范围[1 - 100]
#define	DB_OPERATE_FAILED_ALARM							1	// DB操作失败告警
#define DB_CONN_FAILED_ALARM							2	// DB连接失败
#define REDIS_CONN_FAILED_ALARM							3	// REDIS连接失败
#define MQ_CONN_FAILED_ALARM							4	// MQ连接失败
#define SYS_QUEUE_SUPERTHRESHOLD_ALARM					5	// 系统队列超阈值告警

// 运营告警,范围[101 - 200]
#define PT_SD_SUCCESS_BLW_THRESHOLD_ALARM				101 // 生产平台发送成功率低于阈值告警
#define CHNN_SUBMIT_FAILED_OR_NO_RESPONSE_ALARM			102 // 通道提交失败或无响应
#define	CHNN_FAILURE_ALARM								103 // 通道连续失败
#define CHNN_STATUS_REPORT_FAILED_ALARM					104	// 通道连续状态报告失败
#define CHNN_SUBMIT_NO_RESPONSE_ALARM					105 // 通道连续提交无响应
#define CHNN_SLIDE_WINDOW_ALARM							106 // 通道滑窗重置
#define CHNN_LOGIN_FAILED_ALARM							107 // 通道登录失败告警


const int CHANNEL_SURVIVE_ALARM                         = 0X70000000;
const int CHANNEL_SUCCESS_ALARM                         = 0X70000001;
const int QUENUE_QUANTITY_ALARM                         = 0X70000002;
const int DB_EXCLUETFAIL_ALARM                          = 0X70000003;
const int CHANNEL_LOGIN_FAILED_ALARM                    = 0X70000004;
const int CHANNEL_NO_SUBMITRESP_ALARM                   = 0X70000005;

const int CHANNEL_MSGQUEUESIZE_ALARM                    = 0X70000006;
const int SEND_TO_SMSPSEND_ALARM                        = 0X70000007;

const int REDIS_EXCLUETFAIL_ALARM                          = 0X70000008;



//===================    alarm type    ===================//

//===================    msg type define    =================//
const int MSGTYPE_TIMEOUT                               = 0X80000000;
const int MSGTYPE_HTTPRESPONSE                          = 0X80000001;
const int MSGTYPE_SOCKET_WRITEOVER                      = 0X80000001;



const int MSGTYPE_ALARM_REQ                             = 0X80000010;
const int MSGTYPE_ALARM_RESP                            = 0X80000012;

const int MSGTYPE_DB_UPDATE_QUERY_REQ                   = 0X80000019;
const int MSGTYPE_DB_QUERY_REQ                          = 0X80000020;
const int MSGTYPE_DB_QUERY_RESP                         = 0X80000021;
const int MSGTYPE_DB_NOTQUERY_REQ                       = 0X80000022;
const int MSGTYPE_DB_NOTQUERY_RESP                      = 0X80000023;
const int MSGTYPE_DB_IN_REQ                             = 0X80000024;

const int MSGTYPE_HOSTIP_REQ                            = 0X80000030;
const int MSGTYPE_HOSTIP_RESP                           = 0X80000031;

const int MSGTYPE_LOG_REQ                               = 0X80000040;

const int MSGTYPE_REDIS_REQ                             = 0X80000050;
const int MSGTYPE_REDIS_RESP                            = 0X80000051;
const int MSGTYPE_REDISLIST_REPORT_REQ                  = 0X80000052;
const int MSGTYPE_REDISLIST_REPORT_RESP                  = 0X80000053;
const int MSGTYPE_REDISLIST_REQ      					 = 0X80000054;
const int MSGTYPE_REDISLIST_RESP      					 = 0X80000055;


const int MSGTYPE_CHANNEL_CLOSE_WAIT_TIME_UPDATE		 = 0X80000062;


const int MSGTYPE_TIMEOUT_RESET_SLIDE_WINDOW		 	 = 0X80000063;


const int MSGTYPE_CONTINUE_LOGIN_FAIL_VALUE_UPDATE		 = 0X80000064;


const int MSGTYPE_CHANNEL_SLIDE_WINDOW_UPDATE            = 0X80000065;


const int MSGTYPE_RULELOAD_CHANNEL_LOGIN_STATE_REQ       = 0X80000066;
const int MSGTYPE_RULELOAD_CHANNEL_LOGIN_STATE_RESP      = 0X80000067;

const int MSGTYPE_RULELOAD_CHANNEL_DEL_REQ           	= 0X80000068;
const int MSGTYPE_RULELOAD_CHANNEL_ADD_REQ           	= 0X80000069;


const int MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ           = 0X80000070;
const int MSGTYPE_RULELOAD_USERGW_UPDATE_REQ            = 0X80000071;
const int MSGTYPE_RULELOAD_TEMPLATEGW_UPDATE_REQ        = 0X80000072;
const int MSGTYPE_RULELOAD_TEMPLATE_TPYE_UPDATE_REQ     = 0X80000073;
const int MSGTYPE_RULELOAD_BLACKLIST_UPDATE_REQ         = 0X80000074;
const int MSGTYPE_RULELOAD_SYS_KEYWORDLIST_UPDATE_REQ       = 0X80000075;
const int MSGTYPE_RULELOAD_SIGNEXTNOGW_UPDATE_REQ       = 0X80000076;
const int MSGTYPE_RULELOAD_SMPPPRICE_UPDATE_REQ         = 0X80000077;
const int MSGTYPE_RULELOAD_TEMPLATERULE_UPDATE_REQ      = 0X80000078;
const int MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ      = 0X80000079;
const int MSGTYPE_RULELOAD_PHONE_AREA_UPDATE_REQ        = 0X80000080;

const int MSGTYPE_RULELOAD_CHANNEL_WARN_INFO_UPDATE_REQ = 0X80000081;
const int MSGTYPE_RULELOAD_SYS_USER_UPDATE_REQ          = 0X80000082;
const int MSGTYPE_RULELOAD_SYS_NOTICE_UPDATE_REQ        = 0X80000083;
const int MSGTYPE_RULELOAD_SYS_NOTICE_USER_UPDATE_REQ   = 0X80000084;
const int MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ   		= 0X80000085;
const int MSGTYPE_RULELOAD_SMSACCOUNT_STATE_UPDATE_REQ  = 0X80000086;
const int MSGTYPE_RULELOAD_SMS_SEND_ID_IP_UPDATE_REQ   	= 0X80000087;
const int MSGTYPE_RULELOAD_CHANNEL_ERROR_CODE_UPDATE_REQ= 0X80000088;
const int MSGTYPE_RULELOAD_CLIENTID_AND_SIGN_UPDATE_REQ= 0X80000089;
const int MSGTYPE_DIRECT_SEND_TO_DIRECT_MO_REQ          = 0X8000008A;
const int MSGTYPE_RULELOAD_CHANNELGROUP_UPDATE_REQ	= 0X8000008B;
const int MSGTYPE_RULELOAD_CHANNELBLACKLIST_UPDATE_REQ	= 0X8000008C;
const int MSGTYPE_RULELOAD_CHANNELKEYWORD_UPDATE_REQ	= 0X8000008D;
const int MSGTYPE_RULELOAD_CHANNELWHITELIST_UPDATE_REQ	= 0X8000008E;
const int MSGTYPE_RULELOAD_OPERATERSEGMENT_UPDATE_REQ	= 0X8000008F;
const int MSGTYPE_RULELOAD_NOAUDITKEYWORD_UPDATE_REQ	= 0X80000090;
const int MSGTYPE_RULELOAD_OVERRATEWHITELIST_UPDATE_REQ	= 0X80000091;
const int MSGTYPE_RULELOAD_CUSTOMERORDER_UPDATE_REQ	    = 0X80000092;
const int MSGTYPE_RULELOAD_CHANNEL_EXTENDPORT_UPDATE_REQ =0X80000093;
const int MSGTYPE_RULELOAD_AGENTINFO_UPDATE_REQ			 =0X80000094;
const int MSGTYPE_RULELOAD_OEMCLIENTPOOL_UPDATE_REQ			 =0X80000095;


const int MSGTYPE_CHANNEL_CONN_STATUS					 =0X80000096;

const int MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ		     =0X80000097;

const int MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ	 =0X80000098;

const int MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ          =0X80000099;
const int MSGTYPE_RULELOAD_SYSTEM_ERROR_DESC_EEQ         =0X80000100;

const int MSGTYPE_RULELOAD_COMPONENTCONFIG_UPDATE_REQ	 =0x80000101;

const int MSGTYPE_RULELOAD_SMS_TEMPLATE_UPDATE_REQ			 =0x80000102;
const int MSGTYPE_RULELOAD_ACCESSTOKEN_UPDATE_REQ            =0x80000103;
const int MSGTYPE_RULELOAD_CHANNELTEMPLATE_UPDATE_REQ        =0x80000104;
const int MSGTYPE_RULELOAD_AUDITKEYWORD_UPDATE_REQ           =0x80000105;

const int MSGTYPE_RULELOAD_BUSINESS_WARN_UPDATE_REQ          =0x80000106;
const int MSGTYPE_RULELOAD_BUSINESS_WARN_PHONE_UPDATE_REQ          =0x80000107;

const int MSGTYPE_RULELOAD_OVERRATEKEYWORD_UPDATE_REQ           =0x80000108;
const int MSGTYPE_RULELOAD_KEYWORDTEMPLATERULE_UPDATE_REQ      = 0X80000109;

const int MSGTYPE_RULELOAD_CHANNEL_SEGMENT_UPDATE_REQ          =0x80000110;
const int MSGTYPE_RULELOAD_CLIENT_SEGMENT_UPDATE_REQ          =0x80000111;
const int MSGTYPE_RULELOAD_CHANNEL_SEG_CODE_UPDATE_REQ          =0x80000112;

const int MSGTYPE_RULELOAD_CLOUD_ORDER_UPDATE_REQ 			=0x80000113;
const int MSGTYPE_RULELOAD_CLOUD_PURCHASE_UPDATE_REQ 			=0x80000114;

const int MSGTYPE_RULELOAD_AGENT_ACCOUNT_UPDATE_REQ 			=0x80000115;

const int MSGTYPE_RULELOAD_AUTO_TEMPLATE_UPDATE_REQ				=0x80000116;

const int MSGTYPE_RULELOAD_SMSTESTACCOUNT_UPDATE_REQ   		    =0X80000117;

const int MSGTYPE_RULELOAD_CHANNEL_LOGIN_UPDATE_REQ				=0x80000118;

const int MSGTYPE_RULELOAD_CHANNEL_WHITEKEYWORD_UPDATE_REQ      =0X80000119;

const int MSGTYPE_RULELOAD_CHANNEL_REALTIME_WEIGHTINFO_REQ				=0x80000120;

const int MSGTYPE_RULELOAD_CHANNEL_ATTRIBUTE_WEIGHT_CONFIG_REQ			=0x80000121;

const int MSGTYPE_RULELOAD_CHANNEL_POOL_POLICY_REQ			=0x80000122;

const int MSGTYPE_RULELOAD_AUDIT_CGROUPREFCLIENT_UPDATE_REQ          =0x80000123;

const int MSGTYPE_RULELOAD_AUDIT_CLIENTGROUP_UPDATE_REQ            =0x80000124;

const int MSGTYPE_RULELOAD_AUDIT_KGROUPREFCATEGORY_UPDATE_REQ            =0x80000125;
const int MSGTYPE_RULELOAD_REDIS_ADDR_UPDATE_REQ                       =0x80000126;

const int MSGTYPE_RULELOAD_AUTO_BLACK_TEMPLATE_UPDATE_REQ				=0x80000127;
const int MSGTYPE_RULELOAD_IGNORE_AUDITKEYWORD_UPDATE_REQ               =0x80000132;

const int MSGTYPE_RULELOAD_TIMERSEND_TASKINFO_UPDATE_REQ         =0X80000128;
const int MSGTYPE_RULELOAD_SYS_CGROUPREFCLIENT_UPDATE_REQ			= 0x80000129;
const int MSGTYPE_RULELOAD_SYS_CLIENTGROUP_UPDATE_REQ				= 0x80000130;
const int MSGTYPE_RULELOAD_SYS_KGROUPREFCATEGORY_UPDATE_REQ			= 0x80000131;

const int MSGTYPE_RULELOAD_CHANNELOVERRATE_UPDATE_REQ      = 0X80000133;   //CHANNEL OVERRATE
const int MSGTYPE_RULELOAD_CLIENT_PRIORITY_UPDATE_REQ               = 0x80000134;

const int MSGTYPE_RULELOAD_SYS_AUTHENTICATION_UPDATE_REQ               = 0x80000136;
const int MSGTYPE_RULELOAD_DB_UPDATE_REQ                = 0x80000137;
const int MSGTYPE_RULELOAD_CHANNEL_SGIP_UPDATE_REQ           = 0x80000138;
const int MSGTYPE_RULELOAD_CHANNEL_PROPERTY_LOG_UPDATE_REQ               = 0x80000139;
const int MSGTYPE_RULELOAD_USER_PRICE_LOG_UPDATE_REQ               = 0x8000013A;
const int MSGTYPE_RULELOAD_USER_PROPERTY_LOG_UPDATE_REQ               = 0x8000013B;

const int MSGTYPE_RULELOAD_BLGRP_REF_CATEGORY_UPDATE_REQ               = 0x8000013C;
const int MSGTYPE_RULELOAD_BLUSER_CONFIG_GROUP_UPDATE_REQ               = 0x8000013D;

const int MSGTYPE_RULELOAD_CLIENTFORCEEXIT_UPDATE_REQ                         = 0x8000013E;


//--------------    msg type define of part start   ------------//
const int MSGTYPE_SMSERVICE_REQ_JD						= 0X81000006;
const int MSGTYPE_SMSERVICE_REQ_ML                      = 0X81000007;
const int MSGTYPE_TENCENT_SMSERVICE_REQ                 = 0X81000008;
const int MSGTYPE_SMSERVICE_REQ_2                       = 0X81000009;
const int MSGTYPE_SMSERVICE_REQ                         = 0X81000010;
const int MSGTYPE_REPORTRECIVE_REQ                      = 0X81000011;

const int MSGTYPE_SMSERVICE_TO_DISPATHC_REQ             = 0X81000012;
const int MSGTYPE_DISPATCH_TO_SMSERVICE_RESP            = 0X81000013;
const int MSGTYPE_ACCEPTSOCKET_REQ                      = 0X81000014;
const int MSGTYPE_HTTP_SERVICE_REQ						= 0X81000015;
const int MSGTYPE_HTTP_SERVICE_RESP                     = 0X81000016;
const int MSGTYPE_REPORT_GET_MO_REQ                     = 0X81000017;
const int MSGTYPE_REPORT_GET_REPORT_REQ                 = 0X81000018;
const int MSGTYPE_AUTHENTICATION_REQ                    = 0X81000019;




const int MSGTYPE_DISPATCH_TO_HTTPSEND_REQ              = 0X81000020;
const int MSGTYPE_CHANNEL_SEND_REQ                      = 0X81000021;

const int MSGTYPE_SEND_TO_DELIVERYREPORT_REQ            = 0X81000030;
const int MSGTYPE_SEND_TO_CHARGE_REQ                    = 0X81000040;
const int MSGTYPE_DIRECT_TO_DELIVERYREPORT_REQ          = 0X81000050;
const int MSGTYPE_DISPATCH_TO_DIRECTSEND_REQ            = 0X81000060;
const int MSGTYPE_UPSTREAM_TO_DISPATCH_REQ              = 0X81000070;
const int MSGTYPE_REPORT_TO_DISPATCH_REQ                = 0X81000080;
const int MSGTYPE_DISPATCH_TO_REPORTPUSH_REQ            = 0X81000090;

const int MSGTYPE_REPORT_QUERY_REQ                     	= 0X81000100;
const int MSGTYPE_REPORT_SAVE_POOL_REQ                  = 0X81000101;
const int MSGTYPE_TEST_SMS_SEND_REQ                    	= 0X81000102;

const int MSGTYPE_SEND_TRANSFORM_SEND_REQ               = 0X81000103;
//const int MSGTYPE_SMSERVICE_TRANSFORM_REQ               = 0X81000104;

const int MSGTYPE_TO_DIRECTMO_REQ                       = 0X81000105;
////const int MSGTYPE_DIRECTMO_TO_DELIVERY_REQ              = 0X81000106;

const int MSGTYPE_REPORTRECEIVE_TO_SMS_REQ              = 0X81000200;
const int MSGTYPE_SMS_TO_HTTPSEND_REQ              		= 0X81000201;
const int MSGTYPE_CMPP_CONNECT_REQ              		= 0X81000202;
const int MSGTYPE_CMPP_SUBMIT_REQ              	    	= 0X81000203;
const int MSGTYPE_CMPP_HEART_BEAT_REQ            	    = 0X81000204;
const int MSGTYPE_CMPP_HEART_BEAT_RESP					= 0X81000205;
const int MSGTYPE_CMPP_LINK_CLOSE_REQ              		= 0X81000206;

const int MSGTYPE_TEMPLATE_SMSERVICE_REQ                = 0X81000207;
const int MSGTYPE_TIMERSEND_SMSERVICE_REQ                = 0X81000208;
const int MSGTYPE_REPORTRECEIVE_TO_REPUSH_REQ           = 0X81000209;

const int MSGTYPE_SMGP_CONNECT_REQ              		= 0X81100202;
const int MSGTYPE_SMGP_SUBMIT_REQ              	    	= 0X81100203;
const int MSGTYPE_SMGP_HEART_BEAT_REQ            	    = 0X81100204;
const int MSGTYPE_SMGP_HEART_BEAT_RESP					= 0X81100205;
const int MSGTYPE_SMGP_LINK_CLOSE_REQ              		= 0X81100206;

const int MSGTYPE_SGIP_CONNECT_REQ              		= 0X81200201;
const int MSGTYPE_SGIP_SUBMIT_REQ              	    	= 0X81200202;
const int MSGTYPE_SGIP_LINK_CLOSE_REQ              		= 0X81200203;
const int MSGTYPE_SGIP_RT_REQ							= 0X81200204;
const int MSGTYPE_SGIP_MO_REQ                           = 0X81200205;
const int MSGTYPE_REPORT_GET_AGAIN_REQ					= 0X81200206;
const int MSGTYPE_GET_LINKED_CLIENT_REQ					= 0X81200207;
const int MSGTYPE_GET_LINKED_CLIENT_RESQ				= 0X81200208;
const int MSGTYPE_SGIP_CLIENT_LOGIN_SUC					= 0X81200209;
const int MSGTYPE_MO_GET_AGAIN_REQ						= 0X81200210;








const int MSGTYPE_ACCESS_MO_MSG                         = 0X81000207;


const int MSGTYPE_AUDIT_SERVICE_REQ						= 0X81000300;

const int MSGTYPE_SMPP_CONNECT_REQ              		= 0X81000302;
const int MSGTYPE_SMPP_SUBMIT_REQ              	    	= 0X81000303;
const int MSGTYPE_SMPP_HEART_BEAT_REQ            	    = 0X81000304;
const int MSGTYPE_SMPP_HEART_BEAT_RESP					= 0X81000305;
const int MSGTYPE_SMPP_LINK_CLOSE_REQ              		= 0X81000306;

const int MSGTYPE_TO_CHARGE_REQ              		= 0X81000307;

//////unitythread
const int MSGTYPE_TCP_LOGIN_REQ                         = 0X81000407;
const int MSGTYPE_TCP_LOGIN_RESP                        = 0X81000408;
const int MSGTYPE_TCP_SUBMIT_REQ                        = 0X81000409;
const int MSGTYPE_TCP_SUBMIT_RESP                       = 0X81000410;
const int MSGTYPE_TCP_DELIVER_REQ						= 0X81000411;
const int MSGTYPE_TCP_DISCONNECT_REQ					= 0X81000412;
const int MSGTYPE_HTTPS_TO_UNITY_REQ                    = 0X81000413;
const int MSGTYPE_UNITY_TO_HTTPS_RESP                   = 0X81000414;
const int MSGTYPE_UNITY_TO_HTTPSEND                     = 0X81000415;
const int MSGTYPE_HTTPSEND_TO_SENDSMSPSEND_REQ			= 0X81000416;

const int MSGTYPE_RULELOAD_SMPPSALEPRICE_UPDATE_REQ    	= 0X81000417;

const int MSGTYPE_AFTER_WRITE_FILE_REQ					= 0X81000418;

const int MSGTYPE_AUDIT_RESP_TO_HTTPSEND                = 0X81000419;
const int MSGTYPE_SMSPSEND_TO_HTTPSEND_RESP             = 0X81000420;
const int MSGTYPE_HTTPSEND_TO_AUDIT_REQ                 = 0X81000421;
const int MSGTYPE_HTTPGROUPSENDLIM_TO_AUDIT_REQ         = 0X81000422;
const int MSGTYPE_MQ_PUBLISH_REQ                        = 0X82000000;
const int MSGTYPE_MQ_PUBLISH_DB_REQ                     = 0X82000001;
const int MSGTYPE_MQ_GETMQMSG_REQ                       = 0X82000002;


const int MSGTYPE_GET_CHANNEL_MQ_SIZE_REQ               = 0X82000003;

const int MSGTYPE_CHANNEL_SEND_STATUS_REPORT			= 0X82000004;


const int MSGTYPE_HTTPS_ASYNC_GET						= 0X82000005;
const int MSGTYPE_HTTPS_ASYNC_POST						= 0X82000006;
const int MSGTYPE_HTTPS_ASYNC_RESPONSE					= 0X82000007;
const int MSGTYPE_OLD_NO_PRIORITY_QUEUE_DATA_MOVE_REQ	= 0X82000008;

const int MSGTYPE_MQ_GETMQREBACKMSG_REQ                 = 0X82000009;

const int MSGTYPE_HTTP_ASYNC_POST						= 0X8200000A;


const int MSGTYPE_MONITOR_UDP_SEND				        = 0X83000001;

const int MSGTYPE_ACCESS_STATUS_RPT                     = 0X84000001;

//--------------    msg type define of part start   ------------//

const int MSGTYPE_MQ_QUEUE_CTRL_UPDATE_REQ              = 0X70000000;
const int MSGTYPE_MQ_SWITCH_UPDATE_REQ                  = 0X70000001;
const int MSGTYPE_MQ_THREAD_EXIT_REQ                    = 0X70000002;


//===================    msg type define  end   ================//

//////////////////////////////////////////////////////////////////////////

const int MSGTYPE_SEESION_TO_TEMPLATE_REQ               = 0X90000501;
const int MSGTYPE_TEMPLATE_RESP_TO_SEESION              = 0X91000501;

//////////////////////////////////////////////////////////////////////////

const int MSGTYPE_AUDIT_REQ                             = 0X77777000;
const int MSGTYPE_AUDIT_RSP                             = 0X77777001;

const int MSGTYPE_OVERRATE_CHECK_REQ                    = 0X77778001;
const int MSGTYPE_OVERRATE_CHECK_RSP                    = 0X77778002;
const int MSGTYPE_OVERRATE_SET_RSQ                      = 0X77778003;

//////////////////////////////////////////////////////////////////////////

const int MSGTYPE_SEESION_TO_ROUTER_REQ                 = 0X90000551;
const int MSGTYPE_ROUTER_RESP_TO_SEESION                = 0X91000551;

//////////////////////////////////////////////////////////////////////////


#endif

