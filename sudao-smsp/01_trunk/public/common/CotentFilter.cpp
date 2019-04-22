#include "CotentFilter.h"
#include "UTFString.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>

/********************************************************************
UTF-8，1个汉字占3个字节，扩展B区以后的汉字占4个字节。
GBK，1个汉字占2个字节。
UTF-16，通常汉字占两个字节。
Unicode，一个英文等于两个字节，一个中文（含繁体）等于两个字节，范围是 \u4e00-\u9fff。
\u3400-\u4dbf 中日韩统一扩充a  \uF900-\uFAFF中日韩兼容表意文字
\u20000-2a6df 中日韩统一表意文字扩充B
\u2f800-2fa1f 中日韩兼容表意文字补充

全角小写英文字母[ａ,ｂ,ｃ,ｄ,ｅ,ｆ,ｇ,ｈ,ｉ,ｇ,ｋ,ｌ,ｍ,ｎ,ｏ,ｐ,ｑ,ｒ,ｓ,ｔ,ｕ,ｖ,ｗ,ｘ,ｙ,ｚ]，范围是 \uff41-\uff5a(0x61-7a)

u'\uff41': 'a', u'\uff42': 'b', u'\uff43': 'c', u'\uff44': 'd',
u'\uff45': 'e', u'\uff46': 'f', u'\uff47': 'g', u'\uff48': 'h',
u'\uff49': 'i', u'\uff47': 'j', u'\uff4b': 'k', u'\uff4c': 'l',
u'\uff4d': 'm', u'\uff4e': 'n', u'\uff4f': 'o', u'\uff50': 'p',
u'\uff51': 'q', u'\uff52': 'r', u'\uff53': 's', u'\uff54': 't',
u'\uff55': 'u', u'\uff56': 'v', u'\uff57': 'w', u'\uff58': 'x',
u'\uff59': 'y', u'\uff5a': 'z',

全角大写英文字母[Ａ,Ｂ,Ｃ,Ｄ,Ｅ,Ｆ,Ｇ,Ｈ,Ｉ,Ｊ,Ｋ,Ｌ,Ｍ,Ｎ,Ｏ,Ｐ,Ｑ,Ｒ,Ｓ,Ｔ,Ｕ,Ｖ,Ｗ,Ｘ,Ｙ,Ｚ]，范围是 \uff21-\uff3a(0x41-5a)

u'\uff21': 'A', u'\uff22': 'B', u'\uff23': 'C', u'\uff24': 'D',
u'\uff25': 'E', u'\uff26': 'F', u'\uff27': 'G', u'\uff28': 'H',
u'\uff29': 'I', u'\uff2a': 'J', u'\uff2b': 'K', u'\uff2c': 'L',
u'\uff2d': 'M', u'\uff2e': 'N', u'\uff2f': 'O', u'\uff30': 'P',
u'\uff31': 'K', u'\uff32': 'R', u'\uff33': 'S', u'\uff35': 'T',
u'\uff35': 'U', u'\uff36': 'V', u'\uff37': 'W', u'\uff38': 'X',
u'\uff39': 'Y', u'\uff3a': 'Z',

全角阿拉伯数字[１,２,３,４,５,６,７,８,９,０]，范围是 \uff10-\uff19(0x30-0x39)

u'\uff11': '1', u'\uff12': '2', u'\uff13': '3', u'\uff14': '4',
u'\uff15': '5', u'\uff16': '6', u'\uff17': '7', u'\uff18': '8',
u'\uff19': '9', u'\uff10': '0',

全角符号[１,２,３,４,５,６,７,８,９,０]，范围是 \uff01-\uff20,ff3b-ff40,ff5b-ff5e
FF5f-ff65 --> 0x3c,0x3e,0x2e,0x28,0x29,0x2e

中文数字[一,二,三,四,五,六,七,八,九,零]，范围是 \u4e00-\u96f6

u'\u4e00': '1', u'\u4e8c': '2', u'\u4e09': '3', u'\u56db': '4',
u'\u4e94': '5', u'\u516d': '6', u'\u4e03': '7', u'\u516b': '8',
u'\u4e5d': '9', u'\u96f6': '9'

中文符号转英文符号   
3000-3003 0x20,0x2c,0x2e,0x22.  [ID SP]	、	。	〃    ->   ,."
3008-3011 0x3c,0x3e,0x3c,0x3e,0x28,0x29,0x28,0x29,0x5b,0x5d   〈	〉《	》「	」『	』【	】-> <> <> (	) () []
3013-301e 0x3d,0x28,0x29,0x28,0x29,0x28,0x29,0x28,0x29,0x7e,0x22,0x22 〓	〔	〕	〖	〗	〘	〙	〚	〛	〜〝	〞-> = ()()()()~""
3030   〰 -> ~ 0x22


空格:0x3000,09-0d,0x20,0xff00
****************************************************************************/

namespace filter
{
    ContentFilter::ContentFilter()
    {
        // TODO Auto-generated constructor stub

    }

    ContentFilter::~ContentFilter()
    {
        // TODO Auto-generated destructor stub
    }

	UInt8* ContentFilter::strQ2B(const UInt8* charIn, UInt8* charOut)
	{
		int iSrc = charIn[0]<<24 | charIn[1]<<16 | charIn[2]<<8 | charIn[3];
		int iDest = 0;
		if (iSrc == 0x3000)
		{
			iDest = 0x20;
		}
		else
		{
			iDest = iSrc - 0xfee0;
		}
		
		charOut[0] = iDest >> 24 & 0xff;
		charOut[1] = iDest >> 16 & 0xff;
		charOut[2] = iDest >> 8 & 0xff;
		charOut[3] = iDest & 0xff;
		
		return charOut;
			
	}
	
	UInt8* ContentFilter::strB2Q(const UInt8* charIn, UInt8* charOut)
	{
		int iSrc = charIn[0]<<24 | charIn[1]<<16 | charIn[2]<<8 | charIn[3];
		int iDest = 0;
		if (iSrc == 0x0020)
		{
			iDest = 0x3000;
		}
		else
		{
			iDest = iSrc + 0xfee0;
		}

		charOut[0] = iDest >> 24 & 0xff;
		charOut[1] = iDest >> 16 & 0xff;
		charOut[2] = iDest >> 8 & 0xff;
		charOut[3] = iDest & 0xff;

		return charOut;
		
	}
	
	bool ContentFilter::isChinese(const UInt8* charIn)
	{
		int iSrc = charIn[0]<<24 | charIn[1]<<16 | charIn[2]<<8 | charIn[3];
		if ((iSrc >= 0x4e00 && iSrc <= 0x9fa5) || ((iSrc >= 0x3400 && iSrc <= 0x4dbf)) || (iSrc >= 0x20000 && iSrc <= 0x2a6df) 
			|| (iSrc >= 0x2f800 && iSrc <= 0x2fa1f))
		{
			return true;
		}
		else
			return false;
	}
	
	bool ContentFilter::isDigit(const UInt8* charIn)
	{
		 int iSrc = charIn[0]<<24 | charIn[1]<<16 | charIn[2]<<8 | charIn[3];
		 if (iSrc >= 0x0030 && iSrc <= 0x0039)
		 	return true;
		 else
		 	return false;
	}
	
	bool ContentFilter::isDigitQJ(const UInt8* charIn)
	{
		int iSrc = charIn[0]<<24 | charIn[1]<<16 | charIn[2]<<8 | charIn[3];
		if (iSrc >= 0xff10 && iSrc <= 0xff19)
			return true;
		else
			return false;
	}
	
	bool ContentFilter::isAlpha(const UInt8* charIn)
	{
		int iSrc = charIn[0]<<24 | charIn[1]<<16 | charIn[2]<<8 | charIn[3];
		if ((iSrc >= 0x0041 && iSrc <= 0x005a) || (iSrc >= 0x0061 && iSrc <= 0x007a))
			return true;
		else
			return false;
	}

	bool ContentFilter::isSpace(const UInt8* charIn)
	{
		//空格:0x3000,09-0d,0x20,0xff00
		int iSrc = charIn[0]<<24 | charIn[1]<<16 | charIn[2]<<8 | charIn[3];
		if (iSrc == 0x3000 || (iSrc >= 0x09 && iSrc <= 0x0d) || iSrc == 0x20 || iSrc == 0xff00)
			return true;
		else
			return false;
	}
	
	bool ContentFilter::isAlphaQJ(const UInt8* charIn)
	{
		int iSrc = charIn[0]<<24 | charIn[1]<<16 | charIn[2]<<8 | charIn[3];
		if ((iSrc >= 0xff21 && iSrc <= 0xff3a) || (iSrc >= 0xff41 && iSrc <= 0xff5a))
			return true;
		else
			return false;
	}
	
	bool ContentFilter::isQJ(const UInt8* charIn)
	{
		int iSrc = charIn[0]<<24 | charIn[1]<<16 | charIn[2]<<8 | charIn[3];
		if (iSrc >= 0xff00 && iSrc <= 0xff5e)
		{
			return true;
		}
		else
			return false;
	}
	
	bool ContentFilter::isUpper(const UInt8* charIn)
	{
		int iSrc = charIn[0]<<24 | charIn[1]<<16 | charIn[2]<<8 | charIn[3];
		if ((iSrc >= 0x0041 && iSrc <= 0x005a) || (iSrc >= 0xff21 && iSrc <= 0xff3a))
		{
			return true;
		}
		return false;
	}

	bool ContentFilter::isAscii(const UInt8* charIn)
	{
		int iSrc = charIn[0]<<24 | charIn[1]<<16 | charIn[2]<<8 | charIn[3];
		if (iSrc >= 0x0021 && iSrc <= 0x007e)
		{
			return true;
		}
		return false;
	}

	bool ContentFilter::isChineseSymbol(const UInt8* charIn)
	{
		int iSrc = charIn[0]<<24 | charIn[1]<<16 | charIn[2]<<8 | charIn[3];
		if ((iSrc >= 0x3010 && iSrc <= 0x3011)|| (iSrc >= 0x301d && iSrc <= 0x301e))
		{
			return true;
		}
		else
			return false;
	}
	
	char g_chiToEngSymbol[64]={0x20,0x2c,0x2e,0x22,0x00,0x00,0x00,0x00,0x3c,0x3e,0x3c,0x3e,0x28,0x29,0x28,0x29,0x5b,0x5d,0x00
	,0x3d,0x28,0x29,0x28,0x29,0x28,0x29,0x28,0x29,0x7e,0x22,0x22};
	UInt8* ContentFilter::strChi2EngSymbol(const UInt8* charIn, UInt8* charOut)
	{
		int iSrc = charIn[0]<<24 | charIn[1]<<16 | charIn[2]<<8 | charIn[3];
		int iDest = 0;
		
		if (iSrc == 0x3030)
		{
			iDest = 0x22;
		}
		else
		{
			iDest = g_chiToEngSymbol[iSrc - 0x3000];
		}
		
		charOut[0] = iDest >> 24 & 0xff;
		charOut[1] = iDest >> 16 & 0xff;
		charOut[2] = iDest >> 8 & 0xff;
		charOut[3] = iDest & 0xff;
		return charOut;
		
	}

	/************************************
	only chinese simple,digit(bj),alpha(bj,lower case),chinise simble
	*************************************/
	bool ContentFilter::contentFilter(const string& strSrc, string& strDest, int keyWordregular)
	{
		//convert utf8 to unicode  
		//0:无,1:繁体转简体,2:大写转小写,4:全角转半角;8:去空格;16:中文符号转英文符号;32:删除其他字符
		string strU32;
		int index = 0;
		utils::UTFString utfHelper;
		bool isQ2B = keyWordregular & 0x04;
		bool isUpper2Lower = keyWordregular & 0x02;
		bool isRemSpace = keyWordregular & 0x08;
		bool chiToEngSymb = keyWordregular & 0x10;
		bool delOth = keyWordregular & 0x20;
		 
       	int len = utfHelper.u2u32(strSrc, strU32);
		strDest.reserve(8192);
		char *in = const_cast<char*>(strU32.data());
		char *dest = const_cast<char*>(strDest.data());

		while (len > 0)
		{
			if (isChinese((const unsigned char*)in))
			{
				memcpy(dest, in , 4);
				index+=4;
				dest+=4;
			}
			else
			{
				if (isDigit((const unsigned char*)in))
				{
					memcpy(dest, in , 4);
					index+=4;
					dest+=4;
				}
				else if (isAlpha((const unsigned char*)in))
				{
					memcpy(dest, in , 4);
					index+=4;
					if (isUpper((const unsigned char*)dest) && isUpper2Lower)
					{
						dest[3]+=0x20;
					}
					dest+=4;
					
				}
				else if (isSpace((const unsigned char*)in))
				{
					if (isRemSpace)
					{
						// in + 4;
					}
					else
					{
						memcpy(dest, in , 4);
						index+=4;
						dest+=4;
					}
				}
				else if (isQJ((const unsigned char*)in))
				{
					if (isQ2B)
					{
						strQ2B((const unsigned char*)in, (unsigned char*)dest);
					}
					else
					{
						memcpy(dest, in , 4);
					}
					index+=4;
					if (isUpper((const unsigned char*)dest) && isUpper2Lower)
					{
						dest[3]|=0x20;
					}
					dest+=4;
				}
				else if(isChineseSymbol((const unsigned char*)in))
				{
					if (chiToEngSymb)
					{
						strChi2EngSymbol((const unsigned char*)in, (unsigned char*)dest);
					}
					else
					{
						memcpy(dest, in , 4);
					}
					index+=4;
					dest+=4;
				}
				else if (isAscii((const unsigned char*)in))
				{
					memcpy(dest, in , 4);
					index+=4;
					dest+=4;
				}
				else if (delOth)
				{
					//in + 4
				}
				else
				{
					memcpy(dest, in , 4);
					index+=4;
					dest+=4;
				}
			}

			in+=4;
			len-=4;
		}
		string out;
		if (utfHelper.u322u(strDest, index, out) > 0)
		{
			strDest = out;
			return true;
		}
		else
		{
			//LogError("u322u convert error[%s]\n", strSrc.data());
			strDest = strSrc;
			return false;
		}

		
	}



} /* namespace filter */
