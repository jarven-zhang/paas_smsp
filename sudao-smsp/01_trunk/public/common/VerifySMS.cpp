#include "VerifySMS.h"
#include "main.h"

VerifySMS::VerifySMS()
{
    // TODO Auto-generated constructor stub
    const char* visiblepattern="^[\\+|0-9]{0,1}[0-9]{1,}$";
    regcomp(&_regVisibleNumber, visiblepattern, REG_EXTENDED);
    const char* pattern="^00[1-9]{1,}[0-9]{6,}$";
    regcomp(&_regForeign,pattern,REG_EXTENDED);
    const char* mobilepattern="^(1)[0-9]{10}$";
    regcomp(&_regMobile,mobilepattern,REG_EXTENDED);
	const char* patternPhoneFormat = "^((\\+86)|(86)|( 86)|(0086)|(086))?[1][3456789][0-9]{9}$";
	regcomp(&_regPhoneFormat, patternPhoneFormat, REG_EXTENDED);
}

VerifySMS::~VerifySMS()
{
    // TODO Auto-generated destructor stub
    regfree(&_regVisibleNumber);
    regfree(&_regForeign);
    regfree(&_regMobile);
	regfree(&_regPhoneFormat);
}

bool VerifySMS::isVisiblePhoneFormat(std::string phone)
{
	const size_t nmatch=1;
    regmatch_t pm;
    int status = regexec(&_regVisibleNumber, phone.data(), nmatch, &pm, 0);

    if(status == 0)
    {
        return true;
    }
    else
    {
    	LogWarn("The number is not composed of pure numbers(allowed to contain a + sign)");
        return false;
    }
}

bool VerifySMS::isForeign(std::string target)
{
    const size_t nmatch=1;
    regmatch_t pm;
    int status=regexec(&_regForeign,target.data(),nmatch,&pm,0);

    if(status == REG_NOMATCH)
    {
        return false;
    }
    else if(status == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}
bool VerifySMS::isMobile(std::string target)
{
    const size_t nmatch=1;
    regmatch_t pm;
    int status=regexec(&_regMobile,target.data(),nmatch,&pm,0);

    if(status == REG_NOMATCH)
    {
        return false;
    }
    else if(status == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool VerifySMS::CheckPhoneFormat(std::string& phone)
{	
	const size_t nmatch=1;
    regmatch_t pm;
	int status = regexec(&_regPhoneFormat, phone.data(), nmatch, &pm, 0);
	
	if(status == 0)	//format suc
    {	
		if(phone.substr(0, 2) == "86")
		{
			phone = phone.substr(2);
		}
		else if(phone.substr(0, 3) == "+86")
		{
			phone = phone.substr(3);
		}
		else if(phone.substr(0, 3) == " 86")
		{
			phone = phone.substr(3);
		}
		else if(phone.substr(0, 3) == "086")
		{
			phone = phone.substr(3);
		}
		else if(phone.substr(0, 4) == "0086")
		{	
			phone = phone.substr(4);
		}
		
        return true;
    }
    else	//chinese phone  format err
    {	
		if (true == isForeign(phone))
		{	
			////is gj phonenumber
			return true;
		}
		else
		{
			LogWarn("phone format err. phone[%s]", phone.data());
        	return false;
		}

		/*
		if(phone.substr(0, 2) == "00")	//is gj phoneNumber
		{	
			return true;
		}
		else		//phone error
		{
			LogWarn("phone format err. phone[%s]", phone.data());
        	return false;
		}
		*/
    }
}



