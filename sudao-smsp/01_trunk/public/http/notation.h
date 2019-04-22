#ifndef HTTP_NOTATION_H_
#define HTTP_NOTATION_H_

namespace http
{

    class Notation
    {
    public:
        static bool IsUpAlpha(char ch);
        static bool IsLoAlpha(char ch);
        static bool IsAlpha(char ch);
        static bool IsDigit(char ch);
        static bool IsControl(char ch);
        static bool IsReturn(char ch);
        static bool IsLineFeed(char ch);
        static bool IsSpace(char ch);
        static bool IsTab(char ch);
        static bool IsDQuote(char ch);
        static bool IsSeparator(char ch);
        static bool IsHex(char ch);

        static const char *GetCRLF(const char *begin, const char *end);
        static const char *GetLWS(const char *begin, const char *end);
        static const char *GetText(const char *begin, const char *end);
        static const char *GetToken(const char *begin, const char *end);
        static const char *GetComment(const char *begin, const char *end);
    };

}

#endif /* HTTP_NOTATION_H_ */

