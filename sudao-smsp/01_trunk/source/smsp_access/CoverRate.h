#ifndef __COVER_RATE_H__
#define __COVER_RATE_H__

#include "TempRule.h"
#include "CommClass.h"

typedef map<std::string,STL_MAP_STR > channelOverrateMapType;
class CoverRate
{
public:
	CoverRate();
	virtual ~CoverRate();

	bool isOverRate(std::string& strData);
	bool isKeyWordOverRate(std::string& strData);
	bool isChannelOverRate(std::string& strData, models::TempRule &TempRule);
	
	void setOverRatePer(std::string& strData, long Time);
	void setKeyWordOverRatePer(std::string& strKey, long Time);
	void setChannelOverRatePer(std::string& strKey, long Time);
	void setChannelDayOverRatePer(std::string& strKey, STL_MAP_STR channelDayDataMap);

	void cleanOverRateMap();
	void cleanKeyWordOverRateMap();	
	void cleanChannelOverRateMap();
	
private:
	std::map<std::string,long> m_mapOverRate;
	std::map<std::string,long> m_mapKeyWordOverRate;
	std::map<std::string,long> m_mapChannelOverRate;
	channelOverrateMapType m_mapChannelDayOverRate;
	
};


#endif ///__COVER_RATE_H__

