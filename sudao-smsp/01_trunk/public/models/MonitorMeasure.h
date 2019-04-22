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
	MONITOR_ACCESS_INTERCEPT_SMS, /* access������ ���Ѿ��ϳ�����*/
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
	MONITOR_KEY_TAG,   /* Line��ʽ  TAG*/
	MONITOR_KEY_FIELD, /* Line��ʽ  Field*/
};

enum MonitorValType
{
	MONITOR_VAL_FLOAT, /* ����������*/
	MONITOR_VAL_INT,   /*��������*/
	MONITOR_VAL_BOOL,  /*��������*/
	MONITOR_VAL_STRING,/*�ַ�������*/
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
	bool   bSysSwi;   /* ���ϵͳ����*/
	bool   bUseMQSwi; /* ����Ƿ�ʹ��MQ */
	Int32  iThreadCnt;/* udp�����߳�����*/
	string strAddrIp; /* ������ݿ�IP */
	Int32  iAddrPort; /* ������ݿ�˿�*/
	string strPasswd; /* �������*/
	
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
		monitorColumn	m_mapTableColumns[ MONITOR_MAX_COUNT + 1 ]; /* ����Ϣ*/

};

#endif
