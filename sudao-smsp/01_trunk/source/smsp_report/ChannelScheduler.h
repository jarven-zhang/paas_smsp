#ifndef CHANNELSCHEDULER_H_
#define CHANNELSCHEDULER_H_
#include <map>
#include "models/Channel.h"
#include "models/UserGw.h"
#include "models/TemplateGw.h"
#include "models/TemplateTypeGw.h"

const UInt32 CHL_POLICY_BangDingGuDingTongDao = 1;
const UInt32 CHL_POLICY_ZhongXingQianMingTongDao = 2;
const UInt32 CHL_POLICY_ZiDingYiQianMingTongDao = 3;
const UInt32 CHL_POLICY_DuanXinYanZhengTongDao = 4;
using namespace std;
using namespace models;


class ChannelScheduler
{
public:
    ChannelScheduler();
    virtual ~ChannelScheduler();

    int init();

    void getChannel(models::Channel& smsChannel, int Operater, models::UserGw userGw,
                    models::TemplateGw templateGw,models::TemplateTypeGw templatetypeGw);

    typedef map<int, models::Channel> ChannelMAP;
    ChannelMAP m_ChannelMap;

private:
    bool getChannelByPolicy(Channel &channel, UInt32 operatorstype, Int64 channelID, UInt32 policy);
    bool getChannelById(Int64 channelID, UInt32 operatorstype, Channel &channel);
    bool getChannelByCustom(Channel& channel, UInt32 operatorstype);
    bool getChannelByVerifycode(Channel& channel, UInt32 operatorstype);
    void getBestChannel(Channel &channel, UInt32 operatorstype);


};

#endif /* CHANNELSCHEDULER_H_ */
