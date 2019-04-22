#ifndef __H_MONITOR_MEASURE__
#define __H_MONITOR_MEASURE__

#include "platform.h"
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include "MonitorDetailOption.h"


enum MonitorTableType
{
	MONITOR_RECEIVE_MT_SMS,
	MONITOR_ACCESS_SMS_REPORT,
	MONITOR_ACCESS_MO_SMS,
	MONITOR_ACCESS_AUDIT_SMS,
	MONITOR_ACCESS_INTERCEPT_SMS, /* access拦截数 表已经废除不用*/
	MONITOR_ACCESS_SMS_SUBMIT,
	MONITOR_REBACK_SMS_SUBMIT,
	MONITOR_CHANNEL_SMS_SUBMIT,
	MONITOR_CHANNEL_SMS_SUBRET,
	MONITOR_CHANNEL_SMS_REPORT,
	MONITOR_MIDDLEWARE_ALARM,
	MONITOR_MAX_COUNT,
};

#define MONITOR_ATTR_DEFAULT MONITOR_MAX_COUNT


enum MonitorKeyOption
{
	MONITOR_KEY_FIXED,
	MONITOR_KEY_OPTION,
};

enum MonitorKeyType
{
	MONITOR_KEY_TAG,   /* Line格式  TAG*/
	MONITOR_KEY_FIELD, /* Line格式  Field*/
};

enum MonitorValType
{
	MONITOR_VAL_FLOAT, /* 浮点数类型*/
	MONITOR_VAL_INT,   /*整形数据*/
	MONITOR_VAL_BOOL,  /*布尔类型*/
	MONITOR_VAL_STRING,/*字符串类型*/
};

typedef struct _columnAttr
{
public:
	UInt32 ufieldType;
	UInt32 ufieldState;
	bool   bfieldfixed;
	UInt32 uKeyType;	
}columnAttr;

class CMonitorConf
{
public:
	CMonitorConf(){}
	CMonitorConf& operator =( const CMonitorConf& other )
    {
        bSysSwi = other.bSysSwi;
        bUseMQSwi = other.bUseMQSwi;
        iThreadCnt = other.iThreadCnt;
        iAddrPort = other.iAddrPort;		
        strPasswd = other.strPasswd;
        return *this;
    }

public:
	bool   bSysSwi;   /* 监控系统开关*/
	bool   bUseMQSwi; /* 监控是否使用MQ */
	Int32  iThreadCnt;/* udp发送线程数量*/
	string strAddrIp; /* 监控数据库IP */
	Int32  iAddrPort; /* 监控数据库端口*/
	string strPasswd; /* 监控密码*/
	
};

typedef std::map< std::string, std::string > KeyValMap;

class MonitorMeasure 
{
	#define MONITOR_ATTR( k, ft, kt, vt ) {do{columnAttr attr; \
										  attr.ufieldState = 1;\
										  attr.bfieldfixed = ft;\
										  attr.uKeyType    = kt;\
										  attr.ufieldType  = vt;\
		                                  m_mapTableColumns[MONITOR_ATTR_DEFAULT].insert(make_pair(k, attr));\
									   }while(0);}

	typedef std::map< string, columnAttr > monitorColumn;

public:		

		MonitorMeasure();
		~MonitorMeasure();				
		bool   Init( vector< models::MonitorDetailOption > &vecOption );			
		bool   Encode( UInt32 uMeasurement, KeyValMap &keyValMap, string &strOut );		
		string GetMonitorTableName( UInt32 tableType );
		static bool  GetSysParam( std::map<std::string, std::string> &sysParamMap, CMonitorConf &monitorCnf );

private:
		bool   UpdateMonitorOption( std::vector< models::MonitorDetailOption > &vecOption	);
		bool   FormatkeyString(UInt32 uKeyType,UInt32 uFieldType,string & strVal);

private:		
		monitorColumn	m_mapTableColumns[ MONITOR_MAX_COUNT + 1 ]; /* 表信息*/

};

#endif
