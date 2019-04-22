/************************************************************************************ 
 *
 *  filename		: CChineseConvert.h
 *  brief			: Convert Traditional Chinese to Simple Chinese
 *  date			: 2017-9-12
 *	initial version	: https://github.com/BYVoid/OpenCC/blob/ver.0.4.3/README.md
 *
 ************************************************************************************/  
 
#ifndef __CCHINESE_CONVERT_H__
#define __CCHINESE_CONVERT_H__

#include <string>
#include "opencc.h"

class CChineseConvert
{
public:
	CChineseConvert();
	~CChineseConvert();	
	bool Init();
	bool ChineseTraditional2Simple( const std::string src, std::string& dst );

private:		
	opencc_t	handle; /* ×ª»»¾ä±ú*/
};

#endif
