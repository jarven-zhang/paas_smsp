#ifndef BASE_PROPERTYUTILS_H_
#define BASE_PROPERTYUTILS_H_

//#include "base.h"
#include "property.h"
#include "propertymanager.h"
#include "singleton.h"
#include "defaultpropertymanager.h"

class PropertyUtils
{
    template <typename ValueT>
    class Any : public Property
    {
    public:
        Any()
        {
            value_ = NULL;
            refer_ = NULL;
        }

        Any(const ValueT *value)
        {
            value_ = new ValueT(*value);
            refer_ = NULL;
        }

        virtual ~Any()
        {
            if (value_ != NULL)
            {
                delete value_;
                value_ = NULL;
            }
        }

        virtual Property *Clone()
        {
            return this;
        }

        virtual bool Destroy()
        {
            delete this;
            return true;
        }

        virtual const std::type_info &GetType() const
        {
            return typeid(ValueT);
        }

        virtual void *GetData()
        {
            return (void *) value_;
        }

        virtual void SetData(void *value)
        {
            refer_ = (ValueT *) value;
        }

        ValueT *Value()
        {
            if (value_ != NULL)
            {
                return value_;
            }
            else if (refer_ != NULL)
            {
                return refer_;
            }
            else
            {
                return NULL;
            }
        }

        bool GetValue(ValueT &value)
        {
            ValueT *pValue = Value();
            if (pValue)
            {
                value = *pValue;
            }
            return true;
        }

    private:
        ValueT *value_;
        ValueT *refer_;
    };

public:
    template <typename ValueT>
    static bool SetValue(const std::string &key, ValueT &value)
    {
        return Singleton<DefaultPropertyManager>()->SetValue(key, new Any<ValueT>(&value));
    }

    template <typename ValueT>
    static bool GetValue(const std::string &key, ValueT &value)
    {
        Any<ValueT> any;
        if (Singleton<DefaultPropertyManager>()->GetValue(key, &any))
        {
            value = *any.Value();
            return true;
        }
        return false;
    }

    template <typename ValueT>
    static bool GetValue(const std::string &key, ValueT *&value)
    {
        Any<ValueT> any;
        if (Singleton<DefaultPropertyManager>()->GetValue(key, &any))
        {
            value = any.Value();
            return true;
        }
        return false;
    }
};

//}

//using PropertyUtils;

#endif /* BASE_PROPERTYUTILS_H_ */

