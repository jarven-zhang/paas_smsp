#ifndef CONTENTFILTER_H_
#define CONTENTFILTER_H_
#include "platform.h"
//#include "CLogThread.h"
#include<string>

using namespace std;
namespace filter
{

    class ContentFilter
    {
    public:
        ContentFilter();
        virtual ~ContentFilter();
		bool contentFilter(const string& strSrc, string& strDest, int keyWordregular);
	private:
		UInt8* strQ2B(const UInt8* charIn, UInt8 *charOut);
		UInt8* strB2Q(const UInt8* charIn, UInt8 *charOut);
		bool isChinese(const UInt8* charIn);
		bool isDigit(const UInt8* charIn);
		bool isDigitQJ(const UInt8* charIn);
		bool isAlpha(const UInt8* charIn);
		bool isAlphaQJ(const UInt8* charIn);
		bool isQJ(const UInt8* charIn);
		bool isSpace(const UInt8* charIn);
		bool isUpper(const UInt8* charIn);
		bool isChineseSymbol(const UInt8* charIn);
		UInt8* strChi2EngSymbol(const UInt8* charIn, UInt8* charOut);
		bool isAscii(const UInt8* charIn);

    };

} /* namespace filter */
#endif /* CONTENTFILTER_H_ */
