#ifndef BASE_DEFAULTPROPERTYMANAGER_H_
#define BASE_DEFAULTPROPERTYMANAGER_H_

#ifdef __GXX_EXPERIMENTAL_CXX0X__
#include <unordered_map>
#else
#include <map>
#endif /* __GXX_EXPERIMENTAL_CXX0X__ */

#include "propertymanager.h"

class DefaultPropertyManager : public PropertyManager
{
public:
    DefaultPropertyManager();
    virtual ~DefaultPropertyManager();

public:
    bool GetValue(const std::string &key, Property *property);
    bool SetValue(const std::string &key, Property *property);
    bool DropValue(const std::string &key);

private:
#ifdef __GXX_EXPERIMENTAL_CXX0X__
    typedef std::unordered_map<std::string, Property *> PropertyMap;
#else
    typedef std::map<std::string, Property *> PropertyMap;
#endif /* __GXX_EXPERIMENTAL_CXX0X__ */

private:
    PropertyMap properties_;
};


#endif /* BASE_DEFAULTPROPERTYMANAGER_H_ */

