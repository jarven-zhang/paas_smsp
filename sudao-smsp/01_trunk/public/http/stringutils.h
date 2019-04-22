#ifndef HTTP_STRINGUTILS_H_
#define HTTP_STRINGUTILS_H_

#include <string>

namespace http
{

    class StringUtils
    {
    public:
        static bool ToString(int input, std::string &output);
        static bool ToInteger(const std::string &input, int &output);
    };

}

#endif /* HTTP_STRINGUTILS_H_ */

