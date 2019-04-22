#ifndef ___BASE_BASE64_H_
#define ___BASE_BASE64_H_
#include "platform.h"
#include <string>




class Base64 {
public:
    static std::string Encode(const char *data, UInt32 length);
    static std::string Encode(const std::string &data);

    static std::string Decode(const char *data, UInt32 length);
    static std::string Decode(const std::string &data);
};


#endif /* ___BASE_BASE64_H_ */

