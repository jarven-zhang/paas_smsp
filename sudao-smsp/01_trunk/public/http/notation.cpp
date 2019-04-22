#include "notation.h"

#include <stdlib.h>

namespace http
{

    bool Notation::IsUpAlpha(char ch)
    {
        return (ch >= 'A') && (ch <= 'Z');
    }

    bool Notation::IsLoAlpha(char ch)
    {
        return (ch >= 'a') && (ch <= 'z');
    }

    bool Notation::IsAlpha(char ch)
    {
        return IsUpAlpha(ch) || IsLoAlpha(ch);
    }

    bool Notation::IsDigit(char ch)
    {
        return (ch >= '0') && (ch <= '9');
    }

    bool Notation::IsControl(char ch)
    {
        return (ch >= 0) && (ch <= 31);
    }

    bool Notation::IsReturn(char ch)
    {
        return ch == '\r';
    }

    bool Notation::IsLineFeed(char ch)
    {
        return ch == '\n';
    }

    bool Notation::IsSpace(char ch)
    {
        return ch == ' ';
    }

    bool Notation::IsTab(char ch)
    {
        return ch == '\t';
    }

    bool Notation::IsDQuote(char ch)
    {
        return ch == '"';
    }

    bool Notation::IsSeparator(char ch)
    {
        if ((ch == '(') || (ch == ')') || (ch == '<') || (ch == '>'))
        {
            return true;
        }
        else if ((ch == '@') || (ch == ',') || (ch == ';') || (ch == ':'))
        {
            return true;
        }
        else if ((ch == '\\') || (ch == '"') || (ch == '/') || (ch == '['))
        {
            return true;
        }
        else if ((ch == ']') || (ch == '?') || (ch == '=') || (ch == '{'))
        {
            return true;
        }
        else if ((ch == '}') || (ch == ' ') || (ch == '\t'))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool Notation::IsHex(char ch)
    {
        if ((ch >= 'A') && (ch <= 'F'))
        {
            return true;
        }
        else if ((ch >= 'a') && (ch <= 'f'))
        {
            return true;
        }
        else if ((ch >= '0') && (ch <= '9'))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    const char *Notation::GetCRLF(const char *begin, const char *end)
    {
        if (begin + 1 >= end || begin == NULL)
        {
            return NULL;
        }
        if (begin + 1 < end && IsReturn(begin[0]) && IsLineFeed(begin[1]))
        {
            return begin + 2;
        }
        else
        {
            return NULL;
        }
    }

    const char *Notation::GetLWS(const char *begin, const char *end)
    {
        if (begin >= end || begin == NULL)
        {
            return NULL;
        }
        const char *pos = GetCRLF(begin, end);
        if (pos == NULL)
        {
            pos = begin;
        }
        if (!IsTab(pos[0]) && !IsSpace(pos[0]))
        {
            return NULL;
        }
        while (++pos < end)
        {
            if (!IsTab(pos[0]) && !IsSpace(pos[0]))
            {
                break;
            }
        }
        return pos;
    }

    const char *Notation::GetText(const char *begin, const char *end)
    {
        if (begin >= end || begin == NULL)
        {
            return NULL;
        }
        if (IsControl(begin[0]))
        {
            return NULL;
        }
        while (begin < end)
        {
            if (IsControl(begin[0]))
            {
                break;
            }
            ++begin;
        }
        return begin;
    }

    const char *Notation::GetToken(const char *begin, const char *end)
    {
        if (begin >= end || begin == NULL)
        {
            return NULL;
        }
        if (begin < end)
        {
            if (IsControl(begin[0]) || IsSeparator(begin[0]))
            {
                return NULL;
            }
            while (++begin < end)
            {
                if (IsControl(begin[0]) || IsSeparator(begin[0]))
                {
                    return begin;
                }
            }
        }
        return NULL;
    }

    const char *Notation::GetComment(const char *begin, const char *end)
    {
        if (begin >= end || begin == NULL)
        {
            return NULL;
        }
        return NULL;
    }

}

