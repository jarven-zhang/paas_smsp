#ifndef BASE_PROPERTYMANAGER_H_
#define BASE_PROPERTYMANAGER_H_

#ifndef API
#include <string>
#endif /* API */


class Property;

class PropertyManager
{
public:
    virtual bool GetValue(const std::string &key, Property *property) = 0;
    virtual bool SetValue(const std::string &key, Property *property) = 0;
    virtual bool DropValue(const std::string &key) = 0;
};


#endif /* BASE_PROPERTYMANAGER_H_ */

