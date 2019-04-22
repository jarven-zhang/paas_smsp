#ifndef CHANNELMNG_H_
#define CHANNELMNG_H_
#include <map>
#include "Channellib.h"

#define CHANNEL_CONF_PATH		 			"conf/"

/*����ͨ�������*/
class ChannelLibInfo
{	
public:
	ChannelLibInfo() {
		creater  = NULL;
		handler  = NULL;
	}
	~ChannelLibInfo();

public:	
	smsp::Channellib* creater;
	void*     		  handler; 
	string            path;    /* so ·��*/
	string            name;    /*ͨ��������*/
};

class ChannelMng
{
    typedef std::map<std::string, ChannelLibInfo*> ChannelLibMap;		
    typedef smsp::Channellib* create_t();
	
public:
    ChannelMng();
    virtual ~ChannelMng();
    //create channel
    smsp::Channellib* creatChannellib(std::string channnel, bool use_universalchannel);

    //get channel
    smsp::Channellib* getChannelLib(std::string channelname, bool use_universalchannel = false);
	
    //get channel from libname or channelname
    smsp::Channellib* loadChannelLibByNames(const std::string& channellibname,
                                            const std::string& channelname);

    //delete channel
    int delChannelLib(std::string channelname);

private:    
    ChannelLibMap m_mapLib;
};

#endif /* CHANNELMNG_H_ */
