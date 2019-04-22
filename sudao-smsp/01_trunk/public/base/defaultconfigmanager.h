#ifndef BASE_DEFAULTCONFIGMANAGER_H_
#define BASE_DEFAULTCONFIGMANAGER_H_

#include "xml/xml.h"
#include <map>
#include <stdlib.h>
#include <unistd.h>
#include "defaultparsermanager.h"

class DefaultConfigManager
{
    typedef std::map<std::string, std::string> PathMap;
public:
    DefaultConfigManager();
    ~DefaultConfigManager();
    bool Init();
    std::string GetPath(const std::string &name) const;
    bool LoadConf();
	std::string GetOpenccConfPath();

private:
    std::string GetCwdDirectory() const;
	std::string GetExeDirectory() const;
    std::string GetEnv(const std::string name, const std::string &defaultValue) const;
    bool GetDefaultPath();
    bool Parse(TiXmlElement* element);

private:
    PathMap m_PathMap;
	DefaultParserManager m_DefaultParserManager;
};


#endif /* BASE_DEFAULTCONFIGMANAGER_H_ */

