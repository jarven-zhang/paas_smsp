#ifndef __CHANNELOVERRATE__
#define __CHANNELOVERRATE__
#include <map>
#include <string>
#include <list>


class CoverRate
{
public:
	CoverRate();
	virtual ~CoverRate();

	bool isOverRate(std::string& strData);
	
	void setOverRatePer(std::string& strData, long Time);

	void cleanOverRateMap();
	
private:

	std::map<std::string,long> m_mapOverRate;

	
};


#endif ///__CHANNELOVERRATE__

