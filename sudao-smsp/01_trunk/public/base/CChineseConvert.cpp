#include "CChineseConvert.h"
#include <string.h>
#include <stdio.h>
#include "defaultconfigmanager.h"
#include <iostream>
#include "LogMacro.h"

CChineseConvert::CChineseConvert()
{
	handle = (opencc_t ) -1;		
}

CChineseConvert::~CChineseConvert()
{
	if ( handle != (opencc_t) -1 )
	{
		opencc_close( handle );
	}
}

/* ��ʼ�� */
bool CChineseConvert::Init()
{
	DefaultConfigManager* pConfig = new DefaultConfigManager();
	if( pConfig == NULL || !pConfig->Init())
	{
		LogError("Init CChineseConvert Conf Error");
		return false;
	}

	std::string path =	pConfig->GetOpenccConfPath();
	if( pConfig )
	{
		delete pConfig;
		pConfig = NULL;
	}

	path.append( OPENCC_DEFAULT_CONFIG_TRAD_TO_SIMP );

	handle = opencc_open( path.c_str() );
	if( handle == (opencc_t) -1 )
	{			
		LogError("Init CChineseConvert Handler Error");
		return false;
	}

	/**
		OPENCC_CONVERSION_FAST,
		OPENCC_CONVERSION_SEGMENT_ONLY,
		OPENCC_CONVERSION_LIST_CANDIDATES,
	**/
	//handle.opencc_set_conversion_mode(OPENCC_CONVERSION_FAST); 

	return true;

}

/**
 * 	utf8��ת��������
 * 	@param src        ���룬utf8�ַ���
 * 	@param dst        ��������������ַ���
 * 	
 * 	@return           true:dstΪ��������         false:dstΪsrc
 */
bool CChineseConvert::ChineseTraditional2Simple( const std::string src, std::string& dst )
{	
	char  *outbuf = NULL;
	if ( handle  == (opencc_t) -1 )
	{
		LogError("Handler Not Init");
		goto GO_EXIT;
	}

	if( src.length() <= 0 )
	{
		LogError("Input String length 0");
		goto GO_EXIT;
	}

	outbuf = opencc_convert_utf8( handle, src.c_str(), src.length());
	if ( outbuf == (char *) -1 )
	{
		LogError("convert String %s Error", src.data());
		goto GO_EXIT;
	}

	dst = std::string( outbuf );

	/* �ͷ��ڴ�*/
	free(outbuf);

	//LogDebug("convert %s to %s len(%d)", src.data(), dst.data(), src.length());
	return true;

GO_EXIT:

	dst = src;
	return false;

}


