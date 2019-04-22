#ifndef __PHONE_SECTION_H__
#define __PHONE_SECTION_H__

#include "platform.h"

#include <string>
#include <map>

using std::string;
using std::map;

namespace models
{

class PhoneSection
{
public:
    PhoneSection();
    virtual ~PhoneSection();

public:
    //国内号段
    string phone_section;    //号码段，7位或8位
    UInt32 area_id;    //地区id
    UInt32 code;    //路由代码
};

typedef map<string, PhoneSection> PhoneSectionMap;
typedef PhoneSectionMap::iterator PhoneSectionMapIter;


}

#endif    //__PHONE_SECTION_H__
