#ifndef BASE_PROPERTY_H_
#define BASE_PROPERTY_H_

#ifndef API
#include <typeinfo>
#endif /* API */


class Property 
{
public:
    virtual Property *Clone() = 0;
    virtual bool Destroy() = 0;

    virtual const std::type_info &GetType() const = 0;

    virtual void *GetData() = 0;
    virtual void SetData(void *value) = 0;
};


#endif /* BASE_PROPERTY_H_ */

