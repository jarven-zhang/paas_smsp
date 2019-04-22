#ifndef CHANNELSELECT_H_
#define CHANNELSELECT_H_
#include "models/UserGw.h"
#include "models/TemplateGw.h"
#include "models/TemplateTypeGw.h"
#include "models/TemplateTypeGw.h"
#include "../public/models/SignExtnoGw.h"
#include "../public/models/TempRule.h"
#include <stdlib.h>
#include "../public/models/PhoneSection.h"
#include "CommClass.h"

using namespace models;
using namespace std;

enum
{
	OVERRATE_KEYWORD_RULE_USER_SIGN,
	OVERRATE_KEYWORD_RULE_STAR_SIGN,
	OVERRATE_KEYWORD_RULE_USER_STAR,
};

enum channelOverrateType
{
	CHANNEL_SMSTYPE_SIGN,
	STAR_SMSTYPE_SIGN,
	CHANNEL_STAR_SIGN,
	CHANNEL_SMSTYPE_STAR,
	CHANNEL_STAR_STAR,
	STAR_STAR_SIGN,
	STAR_SMSTYPE_STAR,
};

class TSmsRouterReq;
class SMSSmsInfo;
class ChannelSelect
{
public:
    ChannelSelect();
    virtual ~ChannelSelect();
    bool getUserGw(string userid, models::UserGw& userGw);
    void initParam();

	bool ParseGlobalOverRateSysPara(STL_MAP_STR sysParamMap);
    bool ParseOverRateSysPara(STL_MAP_STR sysParamMap);
	bool ParseKeyWordOverRateSysPara(STL_MAP_STR sysParamMap);
	bool ParseChannelOverRateSysPara(STL_MAP_STR sysParamMap);

	int getGlobalOverRateRule(models::TempRule &TempRule);
    int getOverRateRule(std::string key, models::TempRule &TempRule);
    int getKeyWordOverRateRule(string& strClientId, string strSign, models::TempRule &TempRule, int& keyType);

    void getChannelOverRateRuleAndKey(string strCId, TSmsRouterReq* pInfo, models::TempRule &TempRule, int& keyType);
    void getChannelOverRateRuleAndKey(cs_t strChannelId,
                                    cs_t m_strSmsType,
                                    cs_t m_strSign,
                                    cs_t m_strPhone,
                                    string& strChannelOverRateKey,
                                    models::TempRule &TempRule,
                                    int& keyType);

    int getPhoneArea(string strPhone);
    int getPhoneCode(string strPhone);

	//// return: true get price success ,false get price failed
	//// param salePrice: sale price,costPrice cost price
	bool getSmppPrice(std::string phone, UInt32 channelId,int& salePrice,int& costPrice);

	bool getSmppSalePrice(std::string phone,int& salePrice);

	bool getExtendPort(UInt32 channelId, string& strSign,string& strClientId,string& strExtPort);

	bool getChannelExtendPort(UInt32 channelId,string& strClientId,string& strExtPort);
	bool getChannelGroupVec(string strCliendID, string strSid, string strSmsType, UInt32 uSmsFrom, UInt32 uOperator, std::vector<std::string>& vecChannelGroups,models::UserGw& userGw);

    USER_GW_MAP m_userGwMap;		//KEY: userid_smstype
    std::map<std::string, SignExtnoGw> m_SignextnoGwMap;		//key = signExtnoGw.channelid + "&" + signExtnoGw.sign;
    STL_MAP_STR m_PriceMap;		// key = channelid + "&" + prefix;
    STL_MAP_STR m_SalePriceMap;		// key prefix
    std::map<string ,TempRule> m_TempRuleMap;		//key = userID+ "_" + templetID		//for rest: sid_templetID   for access: clientid_0
    std::map<string ,TempRule> m_ChannelOverrateMap;
    std::map<std::string, PhoneSection> m_PhoneAreaMap;
	std::map<string ,TempRule> m_KeyWordTempRuleMap; ///key client id
	//map<string,RegList> m_OverRateKeyWordMap;
    STL_MAP_STR m_SysParamMap;
    TempRule m_TempRule;		//system overrate param
    TempRule m_KeyWordTempRule;   ///system key word overrate param
    TempRule m_GloabalOverrateRule;
	TempRule m_ChannelSysOverrateRule;

	STL_MAP_STR m_ChannelExtendPortTable;

};

#endif /* CHANNELSELECT_H_ */
