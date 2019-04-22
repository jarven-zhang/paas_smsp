#include "ChannelMng.h"
#include <dlfcn.h>
#include <iterator>
#include <string>
#include <errno.h>
#include <fcntl.h>
#include "UniversalChannel.h"
#include "LogMacro.h"

ChannelMng::ChannelMng()
{
}

ChannelMng::~ChannelMng()
{
	ChannelLibMap::iterator itr = m_mapLib.begin();
	for( ;itr != m_mapLib.end(); )
	{
		delete itr->second;
		m_mapLib.erase(itr++);
	}
}


/*通道库释放析构函数*/
ChannelLibInfo:: ~ChannelLibInfo()
{
	LogWarn(" Close ChannelLib Name[%s], Path[%s]", name.c_str(), path.c_str());

	if( NULL != handler ){
		dlclose(handler);		
		handler = NULL; 
	}

	/* 释放分配的内存空间*/
	if( NULL != creater ) {
		free(creater); 	
		creater = NULL;
	}
}

/*通道创建*/
smsp::Channellib* ChannelMng::creatChannellib( std::string channelname, bool use_universalchannel)
{	
    void* _handle = NULL;
    std::string strLib = "lib";
    std::size_t pos = channelname.find("_");
	
    if (pos != std::string::npos)
    {
        channelname = channelname.substr( 0, pos );
    }

    strLib.append(channelname);
    strLib.append(".so");

	/*防止重复打开库*/
	ChannelLibMap::iterator it = m_mapLib.find( channelname );
	if ( it != m_mapLib.end() ){
		LogWarn("Load Channel So %s Error, Open Before", channelname.c_str());
		return NULL;
	}

    _handle = dlopen( strLib.data(), RTLD_LAZY );
	if(NULL == _handle && false == use_universalchannel){
		LogError("get channellib[%s], strLib[%s]handle failed,err[%s], use_universalchannel: %d",
		  					channelname.c_str(), strLib.c_str(), strerror(errno), use_universalchannel);

		return NULL;
	}
		
	ChannelLibInfo *libInfo = new ChannelLibInfo;
	char conf_path[128];		
	memset(conf_path, 0, sizeof(conf_path));
	snprintf(conf_path, sizeof(conf_path) - 1, "%s%s%s", CHANNEL_CONF_PATH, channelname.c_str(), ".xml");
	if(_handle){
		//LogError("get channellib[%s],strLib[%s]handle failed,err[%s]",
		//	      channelname.c_str(),strLib.c_str(),strerror(errno));

		/*创建方法对象*/
		char* error = NULL ;
		create_t *create_ = (create_t *)dlsym(_handle, "create");
		if (NULL != (error = dlerror()) 
			|| NULL == create_ )
		{
			LogError("dlsym error! err[%s]", error == NULL ? "NULL" : error );
			return NULL;
		}
		
		libInfo->creater = create_();
		if( ! libInfo->creater ){
			LogError("get channellib[%s],strLib[%s]handle failed,err[** Create Obj Error ** ]",
			channelname.c_str(),strLib.c_str());
			delete 	libInfo;
			libInfo->creater = NULL;
		}
		libInfo->handler   = _handle;
		libInfo->path      = strLib;
		libInfo->name      = channelname;
	}
	else {
		if (0 == access(conf_path, F_OK)) {
			LogNotice("load UniversalChannel...\n");
					
			libInfo->creater = new smsp::UniversalChannel;
			libInfo->name = channelname;
													//load UniversalChannel when there is not any plugin to use and the config exist.
		}	
		
	}
	
	
	if(NULL != libInfo->creater)
		m_mapLib[channelname] = libInfo;
	
    return libInfo->creater;
}

smsp::Channellib* ChannelMng::loadChannelLibByNames(const std::string& channellibname,
                                                    const std::string& channelname)
{		
    LogNotice("load libso by libname[%s] channelname[%s]", channellibname.c_str(), channelname.c_str());
    smsp::Channellib* pRet = NULL;
	
	if (!channellibname.empty()) 
    {
        pRet = getChannelLib(channellibname);
    }
		
    if (pRet == NULL) 
    {
        //LogWarn("try channellibname '%s' failed, now try channelname '%s'", channellibname.c_str(), channelname.c_str());
        pRet = getChannelLib(channelname, true);
    }
	
    return pRet;
}

smsp::Channellib* ChannelMng::getChannelLib( std::string channelname, bool use_universalchannel)
{	
	std::size_t pos = channelname.find("_");
    if (pos != std::string::npos)
    {
        channelname = channelname.substr( 0, pos );
    }
	
    ChannelLibMap::iterator it = m_mapLib.find( channelname );
	smsp::Channellib* pLib = NULL;
    if(it != m_mapLib.end())
    {	
    	LogNotice("ChanNelLib %s exist, return it...", channelname.c_str());
		pLib = (*it).second->creater;
        //return (*it).second->creater;
    }	
	else{
		//LogNotice("GetChanNelLib %s Empty, Reload it ******* ", channelname.c_str());
		pLib = creatChannellib(channelname, use_universalchannel);
	}	
	
	if(NULL != pLib){
		smsp::UniversalChannel *p = dynamic_cast<smsp::UniversalChannel *>(pLib);
						
		if(NULL != p){
			char conf_path[128];
			memset(conf_path, 0, sizeof(conf_path));
			snprintf(conf_path, sizeof(conf_path) - 1, "%s%s%s", CHANNEL_CONF_PATH, channelname.c_str(), ".xml");
			if(0 == access(conf_path, F_OK)){
				LogNotice("load config %s to channel %s...\n", conf_path, channelname.c_str());
												
				if(NULL != p && false == p->Init(conf_path)){
					LogError("load config %s to channel %s fail\n", conf_path, channelname.c_str());
										
					delChannelLib(channelname);
					pLib = NULL;
				}
			}	
			else{
				LogError("config %s does not exist, channel %s...\n", conf_path, channelname.c_str());
			}
		}		
		else{
			LogDebug("it is not UniversalChannel-based\n");
		}
	}
	
    return pLib;
}

int ChannelMng::delChannelLib( std::string channelname )
{
	std::size_t pos = channelname.find("_");
    if ( pos != std::string::npos )
    {
        channelname = channelname.substr( 0, pos );
    }
	
    ChannelLibMap::iterator it = m_mapLib.find( channelname );
    if( it != m_mapLib.end())
    {    	
		LogDebug("ChannelLib Del channeName[%s]", channelname.c_str());
		delete it->second;
        m_mapLib.erase(it);
		return 0;
    }

    return -1;
}
