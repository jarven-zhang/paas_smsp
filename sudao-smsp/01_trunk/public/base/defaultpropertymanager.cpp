#include "defaultpropertymanager.h"
#include<stdio.h>

#include "property.h"

DefaultPropertyManager::DefaultPropertyManager()
{
}

DefaultPropertyManager::~DefaultPropertyManager()
{
    for (PropertyMap::iterator i = properties_.begin(); i != properties_.end(); ++i)
    {
        i->second->Destroy();
    }
    properties_.clear();
}

bool DefaultPropertyManager::GetValue(const std::string &key, Property *property)
{
    PropertyMap::iterator iter = properties_.find(key);
    if (iter != properties_.end())
    {
        if (property->GetType() == iter->second->GetType())
        {
            property->SetData(iter->second->GetData());
            return true;
        }
    }
    return false;
}

bool DefaultPropertyManager::SetValue(const std::string &key, Property *property)
{
    PropertyMap::iterator iter = properties_.find(key);
    if (iter != properties_.end())
    {
        iter->second->Destroy();
        iter->second = property->Clone();
    }
    else
    {
        properties_.insert(std::make_pair(key, property->Clone()));
    }
    return true;
}

bool DefaultPropertyManager::DropValue(const std::string &key)
{
    properties_.erase(key);
    return true;
}


