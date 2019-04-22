#include "MonitorMeasure.h"
#include "Comm.h"
#include "LogMacro.h"

MonitorMeasure::MonitorMeasure()
{
}

MonitorMeasure::~MonitorMeasure()
{
}

bool MonitorMeasure::Init( vector< models::MonitorDetailOption > &vecOption )
{
	///////////////////////////////////Tag////////////////////////////////////////////////		
	MONITOR_ATTR("clientid", MONITOR_KEY_FIXED, MONITOR_KEY_TAG, MONITOR_VAL_STRING );
	MONITOR_ATTR("channelid",MONITOR_KEY_FIXED, MONITOR_KEY_TAG, MONITOR_VAL_STRING );	
	MONITOR_ATTR("state",    MONITOR_KEY_FIXED, MONITOR_KEY_TAG, MONITOR_VAL_STRING );		
	MONITOR_ATTR("component_id", MONITOR_KEY_FIXED, MONITOR_KEY_TAG, MONITOR_VAL_STRING );
	MONITOR_ATTR("sign",     MONITOR_KEY_OPTION, MONITOR_KEY_TAG, MONITOR_VAL_STRING  );	
	MONITOR_ATTR("smsfrom",  MONITOR_KEY_OPTION, MONITOR_KEY_TAG, MONITOR_VAL_STRING );
	MONITOR_ATTR("audit_state",  MONITOR_KEY_OPTION, MONITOR_KEY_TAG, MONITOR_VAL_STRING );
	MONITOR_ATTR("errrorcode", MONITOR_KEY_OPTION, MONITOR_KEY_TAG, MONITOR_VAL_STRING );

	///////////////////////////////////Field////////////////////////////////////////////////
	MONITOR_ATTR("smsid",    MONITOR_KEY_FIXED, MONITOR_KEY_FIELD,MONITOR_VAL_STRING );
	MONITOR_ATTR("phone",    MONITOR_KEY_FIXED, MONITOR_KEY_FIELD, MONITOR_VAL_STRING );	
	MONITOR_ATTR("operatorstype", MONITOR_KEY_FIXED, MONITOR_KEY_FIELD, MONITOR_VAL_FLOAT);
	MONITOR_ATTR("smstype",  MONITOR_KEY_OPTION,MONITOR_KEY_FIELD, MONITOR_VAL_FLOAT );
	MONITOR_ATTR("paytype",  MONITOR_KEY_OPTION, MONITOR_KEY_FIELD, MONITOR_VAL_FLOAT);
	MONITOR_ATTR("costtime", MONITOR_KEY_OPTION, MONITOR_KEY_FIELD, MONITOR_VAL_FLOAT);

	///////////////////////////////////DEFAUL////////////////////////////////////////////////	
	MONITOR_ATTR("default",  MONITOR_KEY_OPTION, MONITOR_KEY_FIELD, MONITOR_VAL_STRING );

	UpdateMonitorOption( vecOption );
	
	return true;

}

bool MonitorMeasure::UpdateMonitorOption( vector< models::MonitorDetailOption > &vecOption  )
{
	for( UInt32 i=0; i< MONITOR_MAX_COUNT; i++ )
	{
		m_mapTableColumns[i].clear();		
	}

	vector< models::MonitorDetailOption >::iterator itr = vecOption.begin();
	for(; itr != vecOption.end(); itr++ )
	{
		if( itr->m_uMeasurement < MONITOR_MAX_COUNT )
		{			
			monitorColumn &columnsMap = m_mapTableColumns[ itr->m_uMeasurement ];
			monitorColumn::iterator itr2 = columnsMap.find( itr->m_strFieldName );
			if( itr2 != columnsMap.end())
			{	
				itr2->second.ufieldState = itr->m_uState;
			}
			else
			{
				columnAttr attr;
				attr.bfieldfixed = false; 
				attr.ufieldState = itr->m_uState;
				columnsMap.insert( make_pair(itr->m_strFieldName, attr ));
			}
		}
	}

	return true;
	
}

string MonitorMeasure::GetMonitorTableName( UInt32 tableType )
{
	switch( tableType ) 
	{
		case MONITOR_RECEIVE_MT_SMS:
			return "receive_mt_sms";	
		case MONITOR_ACCESS_SMS_REPORT:
			return "access_sms_report";
		case MONITOR_ACCESS_MO_SMS:
			return "access_mo_sms";
		case MONITOR_ACCESS_AUDIT_SMS:
			return "access_audit_sms";
		case MONITOR_ACCESS_INTERCEPT_SMS:
			return "access_intercept_sms";
		case MONITOR_ACCESS_SMS_SUBMIT:
			return "access_sms_submit";
		case MONITOR_REBACK_SMS_SUBMIT:
			return "reback_sms_submit";
		case MONITOR_CHANNEL_SMS_SUBMIT:
			return "channel_sms_submit";
		case MONITOR_CHANNEL_SMS_SUBRET:
			return "channel_sms_subret";
		case MONITOR_CHANNEL_SMS_REPORT:
			return "channel_sms_report";
		case MONITOR_MIDDLEWARE_ALARM: 	
			return "channel_sms_report";
		default:
			return "";

	}

	return "";
	
}

bool MonitorMeasure::FormatkeyString( UInt32 uKeyType, UInt32 uFieldType, string &strVal )
{
	if(strVal.empty()){
		return false;
	}

	/* 去除空格*/
	Comm::string_replace( strVal, " ",  "");

	/*转义中英文逗号*/	
	Comm::string_replace( strVal, ",", "\\," );
	Comm::string_replace( strVal, "，", "\\,");
	
	if( uKeyType == MONITOR_KEY_FIELD )
	{
		switch( uFieldType )
		{			
			case MONITOR_VAL_BOOL:  
			case MONITOR_VAL_FLOAT:
				break;
				
			case MONITOR_VAL_INT:  
				strVal.append("i");	
				break;
				
			case MONITOR_VAL_STRING:
			{
				string tmp = "\"";
				tmp.append( strVal );
				tmp.append("\"");
				strVal = tmp;
				break;
			}
			default:
				;;
		}
	}	

	return true;
}

bool MonitorMeasure::Encode( UInt32 uMeasurement, KeyValMap &keyValMap, string &strOut )
{	
	if( uMeasurement >= MONITOR_MAX_COUNT )
	{
		LogWarn("Monitor Table Name:[ %d:%s ] Not Find ", uMeasurement, "unkown");
		return false;
	}

	string strTableName = GetMonitorTableName( uMeasurement );
	monitorColumn &localColumn = m_mapTableColumns[ uMeasurement ];
	monitorColumn::iterator itrOption  = localColumn.find("*");

	/* 配置* 并且状态为0 表示整表不可用 */
	if( itrOption != localColumn.end() 
		&& itrOption->second.ufieldState == 0 )
	{
		strOut.append("Monitor Table Name:");
		strOut.append(strTableName);
		strOut.append("Closed");	
		return false;
	}

	string tag= "";
	string field = "";
	
	tag.append( strTableName );
	for( KeyValMap::iterator itrKey = keyValMap.begin(); itrKey != keyValMap.end(); itrKey++ )
	{	
		monitorColumn::iterator itrAttrDef = m_mapTableColumns[MONITOR_ATTR_DEFAULT].find( itrKey->first );
		if( itrAttrDef == m_mapTableColumns[MONITOR_ATTR_DEFAULT].end())
		{
			itrAttrDef = m_mapTableColumns[MONITOR_ATTR_DEFAULT].find("default");
			if( itrAttrDef == m_mapTableColumns[MONITOR_ATTR_DEFAULT].end())
			{
				LogWarn("Can't find Key[ %s or default ] Attr", itrKey->first.data());
				continue;
			}
		}

		/*列定义选项*/
		columnAttr &attrDef = itrAttrDef->second;

		/*查找配置选项*/		
		itrOption = localColumn.find( itrKey->first );
		if( itrOption !=localColumn.end() 
			&& !attrDef.bfieldfixed
			&& itrOption->second.ufieldState == 0 ) 
		{
			continue;
		}

		if(!FormatkeyString(attrDef.uKeyType, attrDef.ufieldType, itrKey->second ))
		{
			continue;
		}

		if( attrDef.uKeyType == MONITOR_KEY_TAG ) 
		{
			tag.append(",");					
			tag.append(itrKey->first);
			tag.append("=");			
			tag.append(itrKey->second );
		}
		else if( attrDef.uKeyType == MONITOR_KEY_FIELD )
		{
			if(!field.empty()){
				field.append(",");
			}
			field.append( itrKey->first );
			field.append( "=" );
			field.append( itrKey->second );
		}		
		
	}

	strOut.append(tag);
	strOut.append("  ");

	if(!field.empty()){
		field.append(",");
	}
	field.append("count=1");
	
	strOut.append( field );

	return true;

}

bool MonitorMeasure::GetSysParam( std::map<std::string, std::string> &sysParamMap, CMonitorConf &monitorCnf )
{
	map< string, string >::iterator itr = sysParamMap.find("MONITOR_SWITCH");
	if( itr != sysParamMap.end())
	{
		vector<string> strVec;
		Comm::split( itr->second, ";", strVec );
		if( strVec.size() <  4 )
		{
			LogWarn("Monitor SysParam Error [%s], size[%d]",
					itr->second.data(), strVec.size());
			return false;
		}
		
		monitorCnf.bSysSwi   = ( atoi(strVec.at(0).data()) == 1 );
		monitorCnf.bUseMQSwi = ( atoi(strVec.at(1).data()) == 1 );
		monitorCnf.iThreadCnt =  atoi(strVec.at(2).data());

		if( strVec.size() >= 4 )
		{
			vector<string> strAddr;
			Comm::split( strVec.at(3), ":", strAddr );

			if( strAddr.size() == 2 )
			{
				monitorCnf.strAddrIp = strAddr.at(0);
				monitorCnf.iAddrPort = atoi(strAddr.at(1).data());
			}
			else
			{
				LogWarn("Get Monitor Addr Error [%s] [%s]",
						itr->second.data(), strVec.at(3).data());
				return false;
			}
		}

		if( strVec.size() >= 5 )
		{
			monitorCnf.strPasswd = strVec.at(4);
		}

	}	

	LogNotice("Monitor SysSwitch[ %s ], UseMq[%s], Addr[%s:%d], ThreadCnt[%d]", 
			  monitorCnf.bSysSwi ? "true" : "false",
			  monitorCnf.bUseMQSwi ? "true" : "false",
			  monitorCnf.strAddrIp.data(), monitorCnf.iAddrPort,
			  monitorCnf.iThreadCnt );

	return true;

}
