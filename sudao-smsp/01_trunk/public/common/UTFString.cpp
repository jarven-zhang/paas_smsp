#include "UTFString.h"
#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <iconv.h>
namespace utils
{
//utf8字符长度1-6，可以根据每个字符第一个字节判断整个字符长度
//0xxxxxxx
//110xxxxx 10xxxxxx
//1110xxxx 10xxxxxx 10xxxxxx
//11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
//111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
//1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
    unsigned char utf8_look_for_table[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3,
                                            3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6,
                                            6, 1, 1
                                          };
#define UTFLEN(x)  utf8_look_for_table[(x)]
    UTFString::UTFString()
    {
        // TODO Auto-generated constructor stub

    }

    UTFString::~UTFString()
    {
        // TODO Auto-generated destructor stub
    }

    int UTFString::isUrl(string& url)
    {
        return 0;

    }
	bool isUtf8(char    strWord)
	{
		unsigned char ch = (unsigned char)strWord;
		if(ch >= 0x80 && ch <= 0xBF)
		{
			return true;
		}
		
		return false;
	}

    int UTFString::IsTextUTF8(const char* str, long length, char *dest)
    {
        int i = 0;
 
		while(i < length)
		{
			unsigned char cur = str[i];

			if(((cur>>7)^0x00) == 0)
			{
				i++;
			}
			else if(((cur>>5)^0x06) == 0)
			{
				if (!isUtf8(str[i+1]))
				{
					memcpy(dest, &str[i], 2);
					return false;
				}
				
				i += 2;
			}
			else if(((cur >> 4) ^ 0x0E) == 0)
			{
				if (i + 3 <= length && isUtf8(str[i+1]) && isUtf8(str[i+2]))
				{
					i += 3;
				}
				else
				{
					memcpy(dest, &str[i], 2);
					return false;
				}
			}
			else if(((cur >> 3) ^ 0x1E) == 0)
			{
				if (i + 4 <= length && isUtf8(str[i+1]) && isUtf8(str[i+2])&& isUtf8(str[i+3]))
				{
					i += 4;
				}
				else
				{
					memcpy(dest, &str[i], 2);
					return false;
				}
			}
			else if (((cur >> 2) ^ 0x3E) == 0)
			{
				if (i + 5 <= length && isUtf8(str[i+1]) && isUtf8(str[i+2])&& isUtf8(str[i+3]) && isUtf8(str[i+4]))
				{
					i += 5;			
				}
				else
				{
					memcpy(dest, &str[i], 2);
					return false;
				}
			}
			else if(((cur >> 1) ^ 0x7E) == 0)
			{
				if (i + 6 <= length && isUtf8(str[i+1]) && isUtf8(str[i+2])&& isUtf8(str[i+3]) && isUtf8(str[i+4]) && isUtf8(str[i+4]))
				{
					i += 6;
				}
				else
				{
					memcpy(dest, &str[i], 2);
					return false;
				}
			}
			else
			{
				memcpy(dest, &str[i], 2);
				return false;
			}

	    }
		
        return true;
    }

    int code_convert(const char *from_charset, const char *to_charset, char *inbuf,
                     size_t& inlen, char *outbuf, size_t& outlen)
    {
        iconv_t cd;
        //int rc;
        char **pin = &inbuf;
        char **pout = &outbuf;

        cd = iconv_open(to_charset, from_charset);
        if (cd == 0)
            return -1;
        memset(outbuf, 0, outlen);
        if ((int)(iconv(cd, pin, &inlen, pout, &outlen)) == -1)
            return -1;
        iconv_close(cd);
        return 0;
    }
    UInt32 UTFString::u2u16(string& in, string& out)
    {
        out = "";
        out.reserve(20481);
        char outbuf[20480] = { 0 };
        size_t outlen = 20480;
        size_t inlen = in.length();
        code_convert("utf-8", "UTF-16BE", const_cast<char*>(in.data()),inlen, outbuf, outlen);
        out.append(outbuf, 20480-outlen);       //new-lng.
        return (UInt32)(20480-outlen);
    }

    UInt32 UTFString::u2ucs2(string& in, string& out)
    {
        out = "";
        out.reserve(20481);
        char outbuf[20480] = { 0 };
        size_t outlen = 20480;
        size_t inlen = in.length();
        code_convert("utf-8", "UCS-2BE", const_cast<char*>(in.data()),inlen, outbuf, outlen);
        out.append(outbuf, 20480-outlen);
        return (UInt32)(20480-outlen);
    }

	UInt32 UTFString::u2u32(const string& in, string& out)
	{
		 out = "";
		 out.reserve(20481);
		 char outbuf[20480] = { 0 };
		 size_t outlen = 20480;
		 size_t inlen = in.length();
		 code_convert("utf-8", "UTF-32BE", const_cast<char*>(in.data()),inlen, outbuf, outlen);
		 out.append(outbuf, 20480-outlen);		 //new-lng.
		 return (unsigned int)(20480-outlen);
	}

    void UTFString::u162u(string& in,string& out)
    {
        out = "";
        out.reserve(20481);
        char outbuf[20480] = {0};
        size_t outlen = 20480;
        size_t inlen = in.length();
        code_convert("UTF-16BE", "utf-8", const_cast<char*>(in.data()),inlen, outbuf, outlen);
        out.append(outbuf, 20480-outlen);
        return;
    }

    void UTFString::u2gb2312(string& in,string& out)
    {
        out = "";
        out.reserve(20481);
        char outbuf[20480] = {0};
        size_t outlen = 20480;
        size_t inlen = in.length();
        code_convert("utf-8", "gb2312", const_cast<char*>(in.data()),inlen, outbuf, outlen);
        out.append(outbuf, 20480-outlen);
        return;
    }

    void UTFString::gb23122u(string& in,string& out)
    {
        out = "";
        out.reserve(20481);
        char outbuf[20480] = {0};
        size_t outlen = 20480;
        size_t inlen = in.length();
        code_convert("gb2312", "utf-8", const_cast<char*>(in.data()),inlen, outbuf, outlen);
        out.append(outbuf, 20480-outlen);
        return;
    }

    UInt32 UTFString::u322u(string& in, size_t inLen, string& out)
    {
        out = "";
        out.reserve(20481);
        char outbuf[20480] = {0};
        size_t outlen = 20480;
        code_convert("UTF-32BE", "utf-8", const_cast<char*>(in.data()),inLen, outbuf, outlen);
        out.append(outbuf, 20480-outlen);
        return (unsigned int)(20480-outlen);;
    }

    void UTFString::u2g(string& in, string& out)
    {
        out = "";
        out.reserve(20481);
        char outbuf[20480] = { 0 };
        size_t outlen = 20480;
        size_t inlen = in.length();
        code_convert("utf-8", "gbk", const_cast<char*>(in.data()),
                     inlen, outbuf, outlen);
        out.append(outbuf);
        return;
    }

    void UTFString::g2u(string& in, string& out)
    {
        out = "";
        out.reserve(20481);
        char outbuf[20480] = { 0 };
        size_t outlen = 20480;
        size_t inlen = in.length();
        code_convert("gbk", "utf-8", const_cast<char*>(in.data()),inlen, outbuf, outlen);
        out.append(outbuf);
        return;
    }

     void UTFString::Ascii2u(string& in,string& out)
    {
        out = "";
        out.reserve(20481);
        char outbuf[20480] = {0};
        size_t outlen = 20480;
        size_t inlen = in.length();
        code_convert("ASCII", "utf-8", const_cast<char*>(in.data()),inlen, outbuf, outlen);
        out.append(outbuf, 20480-outlen);
        return;
    }

    void UTFString::u2Ascii(string& in,string& out)
    {
        out = "";
        out.reserve(20481);
        char outbuf[20480] = {0};
        size_t outlen = 20480;
        size_t inlen = in.length();
        code_convert("utf-8", "ASCII", const_cast<char*>(in.data()),inlen, outbuf, outlen);
        out.append(outbuf, 20480-outlen);
        return;
    }

    int UTFString::getUtf8Length(const string& str)
    {
        int clen = strlen(str.data());
        int len = 0;

        for (const char *ptr = str.data(); *ptr != 0 && len < clen; len++, ptr +=
                 UTFLEN((unsigned char)*ptr));

        return len;
    }

    void UTFString::subUtfString(string& str, string& substr, int start, int end)
    {
        int len = getUtf8Length(str);

        if (start >= len)
            return;
        if (end > len)
            end = len;

        const char *sptr = str.data();
        for (int i = 0; i < start; ++i, sptr += UTFLEN((unsigned char)*sptr));

        const char *eptr = sptr;
        for (int i = start; i < end; ++i, eptr += UTFLEN((unsigned char)*eptr));

        int retLen = eptr - sptr;
        char *retStr = (char*) malloc(retLen + 1);
        memcpy(retStr, sptr, retLen);
        retStr[retLen] = 0;
        substr.append(retStr);
        free(retStr);
        return;
    }

} /* namespace web */
