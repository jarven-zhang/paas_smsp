#ifndef __CMDCODE_H__
#define __CMDCODE_H__

//CMPP
const UInt32 CMPP_CONNECT							= 0x00000001;
const UInt32 CMPP_CONNECT_RESP						= 0x80000001;
const UInt32 CMPP_TERMINATE							= 0x00000002;
const UInt32 CMPP_TERMINATE_RESP					= 0x80000002;
const UInt32 CMPP_SUBMIT							= 0x00000004;
const UInt32 CMPP_SUBMIT_RESP						= 0x80000004;
const UInt32 CMPP_DELIVER							= 0x00000005;
const UInt32 CMPP_DELIVER_RESP						= 0x80000005;
const UInt32 CMPP_QUERY								= 0x00000006;
const UInt32 CMPP_QUERY_RESP						= 0x80000006;
const UInt32 CMPP_CANCEL							= 0x00000007;
const UInt32 CMPP_CANCEL_RESP						= 0x80000007;
const UInt32 CMPP_ACTIVE_TEST						= 0x00000008;
const UInt32 CMPP_ACTIVE_TEST_RESP					= 0x80000008;
const UInt32 CMPP_FWD								= 0x00000009;
const UInt32 CMPP_FWD_RESP							= 0x80000009;
const UInt32 CMPP_MT_ROUTE							= 0x00000010;
const UInt32 CMPP_MT_ROUTE_RESP						= 0x80000010;
const UInt32 CMPP_MO_ROUTE							= 0x00000011;
const UInt32 CMPP_MO_ROUTE_RESP						= 0x80000011;
const UInt32 CMPP_GET_ROUTE							= 0x00000012;
const UInt32 CMPP_GET_ROUTE_RESP					= 0x80000012;
const UInt32 CMPP_MT_ROUTE_UPDATE					= 0x00000013;
const UInt32 CMPP_MT_ROUTE_UPDATE_RESP				= 0x80000013;
const UInt32 CMPP_MO_ROUTE_UPDATE					= 0x00000014;
const UInt32 CMPP_MO_ROUTE_UPDATE_RESP				= 0x80000014;
const UInt32 CMPP_PUSH_MT_ROUTE_UPDATE				= 0x00000015;
const UInt32 CMPP_PUSH_MT_ROUTE_UPDATE_RESP			= 0x80000015;
const UInt32 CMPP_PUSH_MO_ROUTE_UPDATE				= 0x00000016;
const UInt32 CMPP_PUSH_MO_ROUTE_UPDATE_RESP			= 0x80000016;
const UInt32 CMPP_GET_MO_ROUTE						= 0x00000017;
const UInt32 CMPP_GET_MO_ROUTE_RESP					= 0x80000017;

//SGIP
const UInt32 SGIP_BIND								= 0x00000001;
const UInt32 SGIP_BIND_RESP							= 0x80000001;
const UInt32 SGIP_UNBIND							= 0x00000002;
const UInt32 SGIP_UNBIND_RESP						= 0x80000002;
const UInt32 SGIP_SUBMIT							= 0x00000003;
const UInt32 SGIP_SUBMIT_RESP						= 0x80000003;
const UInt32 SGIP_DELIVER							= 0x00000004;	//MO 
const UInt32 SGIP_DELIVER_RESP						= 0x80000004;	//MO response
const UInt32 SGIP_REPORT							= 0x00000005;	//RT
const UInt32 SGIP_REPORT_RESP						= 0x80000005;	//RT  response
const UInt32 SGIP_ADDSP								= 0x00000006;
const UInt32 SGIP_ADDSP_RESP						= 0x80000006;
const UInt32 SGIP_MODIFYSP							= 0x00000007;
const UInt32 SGIP_MODIFYSP_RESP						= 0x80000007;
const UInt32 SGIP_DELETESP							= 0x00000008;
const UInt32 SGIP_DELETESP_RESP						= 0x80000008;
const UInt32 SGIP_QUERYROUTE						= 0x00000009;
const UInt32 SGIP_QUERYROUTE_RESP					= 0x80000009;
const UInt32 SGIP_ADDTELESEG						= 0x0000000a;
const UInt32 SGIP_ADDTELESEG_RESP					= 0x8000000a;
const UInt32 SGIP_MODIFYTELESEG						= 0x0000000b;
const UInt32 SGIP_MODIFYTELESEG_RESP				= 0x8000000b;
const UInt32 SGIP_DELETETELESEG						= 0x0000000c;
const UInt32 SGIP_DELETETELESEG_RESP				= 0x8000000c;
const UInt32 SGIP_ADDSMG							= 0x0000000d;
const UInt32 SGIP_ADDSMG_RESP						= 0x8000000d;
const UInt32 SGIP_MODIFYSMG							= 0x0000000e;
const UInt32 SGIP_MODIFYSMG_RESP					= 0x0000000e;
const UInt32 SGIP_DELETESMG							= 0x0000000f;
const UInt32 SGIP_DELETESMG_RESP					= 0x8000000f;
const UInt32 SGIP_CHECKUSER							= 0x00000010;
const UInt32 SGIP_CHECKUSER_RESP					= 0x80000010;
const UInt32 SGIP_USERRPT							= 0x00000011;
const UInt32 SGIP_USERRPT_RESP						= 0x80000011;
const UInt32 SGIP_TRACE								= 0x00001000;
const UInt32 SGIP_TRACE_RESP						= 0x80001000;

//SMGP
const UInt32 SMGP_LOGIN								= 0x00000001;
const UInt32 SMGP_LOGIN_RESP						= 0x80000001;
const UInt32 SMGP_SUBMIT							= 0x00000002;
const UInt32 SMGP_SUBMIT_RESP						= 0x80000002;
const UInt32 SMGP_DELIVER							= 0x00000003;
const UInt32 SMGP_DELIVER_RESP						= 0x80000003;
const UInt32 SMGP_ACTIVE_TEST						= 0x00000004;
const UInt32 SMGP_ACTIVE_TEST_RESP					= 0x80000004;
const UInt32 SMGP_TERMINATE							= 0x00000006;
const UInt32 SMGP_TERMINATE_RESP					= 0x80000006;
const UInt32 SMGP_QUERY								= 0x00000007;
const UInt32 SMGP_QUERY_RESP						= 0x80000007;

const UInt32 Forward								= 0x00000005;
const UInt32 Forward_Resp							= 0x80000005;
const UInt32 Query_TE_Route							= 0x00000008;
const UInt32 Query_TE_Route_Resp					= 0x80000008;
const UInt32 Query_SP_Route							= 0x00000009;
const UInt32 Query_SP_Route_Resp					= 0x80000009;
const UInt32 Payment_Request						= 0x0000000A;
const UInt32 Payment_Request_Resp					= 0x8000000A;
const UInt32 Payment_Affirm							= 0x0000000B;
const UInt32 Payment_Affirm_Resp					= 0x8000000B;
const UInt32 Query_UserState						= 0x0000000C;
const UInt32 Query_UserState_Resp					= 0x8000000C;
const UInt32 Get_All_TE_Route						= 0x0000000D;
const UInt32 Get_All_TE_Route_Resp					= 0x8000000D;
const UInt32 Get_All_SP_Route						= 0x0000000E;
const UInt32 Get_All_SP_Route_Resp					= 0x8000000E;
const UInt32 Update_TE_Route						= 0x0000000F;
const UInt32 Update_TE_Route_Resp					= 0x8000000F;
const UInt32 Update_SP_Route						= 0x00000010;
const UInt32 Update_SP_Route_Resp					= 0x80000010;
const UInt32 Push_Update_TE_Route					= 0x00000011;
const UInt32 Push_Update_TE_Route_Resp				= 0x80000011;
const UInt32 Push_Update_SP_Route					= 0x00000012;
const UInt32 Push_Update_SP_Route_Resp				= 0x80000012;

//SMPP
const UInt32 GENERIC_NACK							= 0X80000000;
const UInt32 BIND_RECEIVER							= 0X00000001;
const UInt32 BIND_RECEIVER_RESP						= 0X80000001;
const UInt32 BIND_TRANSMITTER						= 0X00000002;
const UInt32 BIND_TRANSMITTER_RESP					= 0X80000002;
const UInt32 QUERY_SM								= 0X00000003;
const UInt32 QUERY_SM_RESP							= 0X80000003;
const UInt32 SUBMIT_SM								= 0X00000004;
const UInt32 SUBMIT_SM_RESP							= 0X80000004;
const UInt32 DELIVER_SM								= 0X00000005;
const UInt32 DELIVER_SM_RESP						= 0X80000005;
const UInt32 UNBIND									= 0X00000006;          
const UInt32 UNBIND_RESP							= 0X80000006;     
const UInt32 REPLACE_SM								= 0X00000007;
const UInt32 REPLACE_SM_RESP						= 0X80000007;
const UInt32 CANCEL_SM								= 0X00000008;
const UInt32 CANCEL_SM_RESP							= 0X80000008;
const UInt32 BIND_TRANSCEIVER						= 0X00000009;
const UInt32 BIND_TRANSCEIVER_RESP					= 0X80000009;
const UInt32 OUTBIND								= 0X0000000B;
const UInt32 ENQUIRE_LINK							= 0X00000015;
const UInt32 ENQUIRE_LINK_RESP						= 0X80000015;
const UInt32 SUBMIT_MULTI							= 0x00000021;
const UInt32 SUBMIT_MULTI_RESP						= 0x80000021;
const UInt32 DATA_SM								= 0x00000103;
const UInt32 DATA_SM_RESP							= 0x80000103;

#endif ////

