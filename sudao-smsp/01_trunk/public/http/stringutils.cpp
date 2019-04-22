#include "stringutils.h"

#include "notation.h"

namespace http
{

    bool StringUtils::ToString(int input, std::string &output)
    {
        /*    output.clear();
            output.reserve(15);
            if (input == 0) {
                output.append(1, '\0');
            }*/
        return true;
    }

    bool StringUtils::ToInteger(const std::string &input, int &output)
    {
        output = 0;
        for (int i = 0; i < (int) input.size(); ++i)
        {
            if (!Notation::IsDigit(input[i]))
            {
                return false;
            }
            output = output * 10 + input[i] - '0';
        }
        return true;
    }

}

