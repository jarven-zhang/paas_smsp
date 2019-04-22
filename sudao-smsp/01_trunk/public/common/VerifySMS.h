#ifndef VERIFYSMS_H_
#define VERIFYSMS_H_
#include<string>
#include <regex.h>
#include "boost/regex.hpp"


class VerifySMS
{
public:
    VerifySMS();
    virtual ~VerifySMS();
    bool isForeign(std::string target);
    bool isMobile(std::string target);
	bool CheckPhoneFormat(std::string& phone);
	bool isVisiblePhoneFormat(std::string phone);
private:
    regex_t _regForeign;
    regex_t _regMobile;
	regex_t _regPhoneFormat;
	regex_t _regVisibleNumber;
};


#endif /* VERIFYSMS_H_ */
