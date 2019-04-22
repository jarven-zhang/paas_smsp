#ifndef BASE_DEFAULTPARSERMANAGER_H_
#define BASE_DEFAULTPARSERMANAGER_H_

#include <map>
#include <list>
#include "xml/xml.h"
#include "platform.h"
#include "propertyutils.h"

class Parser
{
public:
    virtual bool Parse(const std::string &key, TiXmlElement *element) = 0 ;
};

template <typename ValueT>
class SimpleParser: public Parser
{
public:
    virtual bool Parse(const std::string &key, TiXmlElement *element)
    {
        ValueT value;
        if (Parse(element, value))
        {
            PropertyUtils::SetValue(key, value);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool Parse(TiXmlElement *element, ValueT &value)
    {
        const char *text = element->GetText();
        if (text != NULL)
        {
            std::istringstream stream(text);
            if (stream >> value)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return true;
        }
    }
};

class ListParser: public Parser
{
public:
    ListParser(){}
    virtual ~ListParser(){}

protected:
    virtual bool Parse(const std::string &key, TiXmlElement *element);
};

class MapParser: public Parser
{
public:
    MapParser(){}
    virtual ~MapParser(){}

protected:
    virtual bool Parse(const std::string &key, TiXmlElement *element);
};

class DefaultParserManager
{
public:
    DefaultParserManager();
    virtual ~DefaultParserManager();
    Parser *GetParser(const std::string &type);

private:
    typedef std::map<std::string, Parser *> ParserMap;

    ParserMap parsers_;

	SimpleParser<std::string> stringParser;
	SimpleParser<UInt32> integerParser;
	SimpleParser<UInt64> longParser;
	SimpleParser<double> doubleParser;
	ListParser listParser;
	MapParser mapParser;
};

#endif /* BASE_DEFAULTPARSERMANAGER_H_ */

