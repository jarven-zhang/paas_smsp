#ifndef __C_URLCODE_H__
#define __C_URLCODE_H__
/*
 * UrlCode.h
 *
 *  Created on: 2014-4-18
 *      Author: andrew
 */

//#include "core/core.h"
#include <string>
namespace http
{

    class UrlCode
    {
    public:
        UrlCode();
        virtual ~UrlCode();
        static std::string UrlDecode(const std::string& str);
        static std::string UrlEncode(const std::string& str);
    };

} /* namespace web */
#endif /* URLCODE_H_ */
