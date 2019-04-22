#include "defaultconfigmanager.h"
#include "defaultparsermanager.h"
#include <iostream>

DefaultConfigManager::DefaultConfigManager()
{
}

DefaultConfigManager::~DefaultConfigManager()
{
}

bool DefaultConfigManager::Init()
{
    if(false == GetDefaultPath())
    {
    	printf("GetDefaultPath is failed!");
        return false;
    }
    return true;
}

std::string DefaultConfigManager::GetCwdDirectory() const
{
    char buffer[128];
    if (buffer == getcwd(buffer, 128))
    {
        return buffer;
    }
    else
    {
    	printf("GetCwdDirectory is failed!");
        return ".";
    }
}

std::string DefaultConfigManager::GetExeDirectory() const
{
	std::string strBinpath = "";
	char link[128] = {0};
	char path[128] = {0};
    snprintf(link, 128,"/proc/%d/exe", getpid());
    if(-1 == readlink(link, path, sizeof(path)))
    {
    	printf("GetExeDirectory is failed!");
    	return ".";
    }
    
    return path;
}

std::string DefaultConfigManager::GetEnv(const std::string name, const std::string &defaultValue) const
{
    char *value = getenv(name.data());
    if (value == NULL)
    {
        return defaultValue;
    }
    else
    {
        return value;
    }
}

bool DefaultConfigManager::GetDefaultPath()
{
    std::string workPath = "";
    std::string binPath = "";
    std::string exePath = GetExeDirectory();
    std::size_t pos0 = exePath.find_last_of('/');
    if (pos0 != std::string::npos)
    {
        binPath = exePath.substr(0, pos0);
    }
    else
    {
		printf("GetDefaultPath 0 is failed!");
        return false;
    }

    std::size_t pos1 = binPath.find_last_of('/');
    if (pos1 != std::string::npos)
    {
        workPath = binPath.substr(0, pos1);
    }
    else
    {
    	printf("GetDefaultPath 1 is failed!");
        return false;
    }

    m_PathMap["cwd"] = GetCwdDirectory();
    m_PathMap["home"] = GetEnv("PATH_HOME", "/home/");
    m_PathMap["exe"] = GetEnv("PATH_EXE", exePath);
    m_PathMap["bin"] = GetEnv("PATH_BIN", binPath);
    m_PathMap["work"] = GetEnv("PATH_WORK", workPath);
    m_PathMap["conf"] = GetEnv("PATH_CONF", workPath+"/conf/");
    m_PathMap["logs"] = GetEnv("PATH_LOG", workPath+"/logs/");
    m_PathMap["lib"] = GetEnv("PATH_LIB", workPath+"/lib/");
    m_PathMap["bill"] = GetEnv("PATH_BILL", workPath+"/bill/");
	m_PathMap["sqlfile"] = GetEnv("PATH_SQLFILE", workPath+"/sqlfile/");
    return true;
}

std::string DefaultConfigManager::GetPath(const std::string &name) const
{
    PathMap::const_iterator iter = m_PathMap.find(name);
    if (iter != m_PathMap.end())
    {
        return iter->second;
    }
    else
    {
    	printf("GetPath is failed!");
        return ".";
    }
}

bool DefaultConfigManager::LoadConf()
{
    std::string path = GetPath("conf");
    std::string filename = path + "/config.xml";
    TiXmlDocument myDocument;
    myDocument.LoadFile(filename.data());
    TiXmlElement *element = myDocument.RootElement();
    return Parse(element);
}

std::string DefaultConfigManager::GetOpenccConfPath()
{
	return GetPath("conf") + "opencc/";
}

bool DefaultConfigManager::Parse(TiXmlElement *element)
{
    while (element != NULL)
    {
        std::string tagName = element->Value();
        if (tagName != "key")
        {
            break;
        }
        std::string key = element->GetText();

        element = element->NextSiblingElement();

        if (element == NULL)
        {
            break;
        }
        std::string type = element->Value();
        DefaultParserManager* parsermanager = &m_DefaultParserManager;
        Parser *parser = parsermanager->GetParser(type);
        if (parser != NULL && !parser->Parse(key, element))
        {
            break;
        }
        element = element->NextSiblingElement();
    }
    return true;
}


