#include "defaultparsermanager.h"


template <typename ValueT>
bool ParseList(const std::string &key, TiXmlElement *element)
{
    std::list<ValueT> value;
    SimpleParser<ValueT> parser;
    ValueT obj;
    for (TiXmlElement *elem = element->FirstChildElement(); elem != NULL; elem = elem->NextSiblingElement())
    {
        if (parser.Parse(elem, obj))
        {
            value.push_back(obj);
        }
        else
        {
            return false;
        }
    }
    PropertyUtils::SetValue(key, value);
    return true;
}

bool ListParser::Parse(const std::string &key, TiXmlElement *element)
{
    std::string type = "string";
    const char *attribute = element->Attribute("type");
    if (attribute != NULL)
    {
        type = attribute;
    }
    if (type == "string")
    {
        return ParseList<std::string>(key, element);
    }
    else if (type == "integer")
    {
        return ParseList<UInt32>(key, element);
    }
	else if (type == "long")
	{
		return ParseList<UInt64>(key, element);
	}
    else if (type == "float")
    {
        return ParseList<double>(key, element);
    }
    return false;
}

template <typename ValueFirst,typename ValueSecond>
bool ParseMap(const std::string &key, TiXmlElement *element)
{
    std::map<ValueFirst,ValueSecond> map;
    SimpleParser<ValueFirst> parser1;
    SimpleParser<ValueSecond> parser2;
    ValueFirst obj1;
    ValueSecond obj2;
    for (TiXmlElement *elem = element->FirstChildElement(); elem != NULL; elem = elem->NextSiblingElement())
    {
        if (!parser1.Parse(elem, obj1))
        {
            return false;
        }
        elem = elem ->NextSiblingElement();
        if (!parser2.Parse(elem,obj2))
        {
            return false;
        }
        map.insert(std::pair<ValueFirst,ValueSecond>(obj1,obj2));
    }
    PropertyUtils::SetValue(key, map);
    return true;
}

template<typename ValueT>
bool InfoFirst(const std::string &key, std::string value, TiXmlElement *element)
{
    if (value == "string")
    {
        return ParseMap<ValueT, std::string>(key,element);
    }
    else if(value == "integer")
    {
        return ParseMap<ValueT, UInt32>(key,element);
    }
	else if (value == "long")
	{
		return ParseMap<ValueT, UInt64>(key,element);
	}
    else if(value == "float")
    {
        return ParseMap<ValueT, double>(key,element);
    }
    return false;
}

bool MapParser::Parse(const std::string &key, TiXmlElement *element)
{
    std::string type = "string";
    std::string value = "string";

    const char *attribute = NULL;
    attribute = element->Attribute("key");
    if (attribute != NULL)
    {
        type = attribute;
    }
    attribute = element->Attribute("value");
    if (attribute != NULL)
    {
        value = attribute;
    }

    if (type == "string")
    {
        return InfoFirst<std::string>(key,value,element);
    }
    else if (type == "integer")
    {
        return InfoFirst<UInt32>(key,value,element);
    }
	else if (type == "long")
	{
		return InfoFirst<UInt64>(key,value,element);
	}
    else if (type == "float")
    {
        return InfoFirst<double>(key,value,element);
    }
    return false;
}
DefaultParserManager::DefaultParserManager()
{
    parsers_["string"] = &stringParser;
    parsers_["integer"] = &integerParser;
	parsers_["long"] = &longParser;
    parsers_["double"] = &doubleParser;
    parsers_["list"] = &listParser;
    parsers_["map"] = &mapParser;
}

DefaultParserManager::~DefaultParserManager()
{
}

Parser *DefaultParserManager::GetParser(const std::string &type)
{
    ParserMap::iterator it = parsers_.find(type);
    if (it == parsers_.end())
    {
        return NULL;
    }
    else
    {
        return it->second;
    }
}

