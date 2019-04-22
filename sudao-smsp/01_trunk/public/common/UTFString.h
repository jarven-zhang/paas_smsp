#ifndef UTFSTRING_H_
#define UTFSTRING_H_
#include "platform.h"

#include<string>
using namespace std;
namespace utils
{

    class UTFString
    {
    public:
        UTFString();
        virtual ~UTFString();
        int getUtf8Length(const string& str);
        int isUrl(string& str);
        void subUtfString(string& str, string& substr,int start, int end);
        UInt32 u2u16(string& in,string& out);
        UInt32 u2ucs2(string& in, string& out);
        UInt32 u2u32(const string& in, string& out);
        void u162u(string& in,string& out);
        UInt32 u322u(string& in, size_t inLen, string& out);
        void u2g(string& in,string& out);
        void g2u(string& in,string& out);
        void u2gb2312(string& in,string& out);
        void gb23122u(string& in,string& out);
        void Ascii2u(string& in, string& out);
        void u2Ascii(string& in,string& out);
        int IsTextUTF8(const char* str, long length, char *dest);
    };

} /* namespace web */
#endif /* UTFSTRING_H_ */
