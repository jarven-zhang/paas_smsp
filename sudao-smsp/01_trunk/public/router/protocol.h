#ifndef __C_PROTOCOL_H__
#define __C_PROTOCOL_H__

#include <stdio.h>
#include <set>
#include "ChannelSegment.h"
#include "boost/regex.hpp"
#include "PhoneSection.h"
#include "SmsAccount.h"

typedef enum
{
    CHANNEL_GET_SUCCESS                    = 0,
    CHANNEL_GET_FAILURE_OVERFLOW           = 1,
    CHANNEL_GET_FAILURE_NO_CHANNELGROUP    = 2,
    CHANNEL_GET_FAILURE_LOGINSTATUS        = 3,
    CHANNEL_GET_FAILURE_NOT_RESEND_MSG     = 4,
    CHANNEL_GET_FAILURE_OVERRATE           = 5,
    CHANNEL_GET_FAILURE_OTHER              = 99,
}CHANNEL_GET_RESULT_CODE;


typedef std::list<std::string> stl_list_str;
typedef std::list<boost::regex> stl_list_regex;
typedef std::list<Int32> stl_list_int32;
typedef std::set<UInt64> UInt64Set;
typedef std::list<models::ChannelSegment> ChannelSegmentList;
typedef std::map<std::string, ChannelSegmentList> stl_map_str_ChannelSegmentList;
typedef std::multimap<int, models::ChannelSegment> stl_multimap_str_ChannelSegment;
typedef std::map<std::string, models::PhoneSection> stl_map_str_PhoneSection;
typedef std::map<std::string, stl_list_str> stl_map_str_list_str;
typedef std::map<std::string, SmsAccount> stl_map_str_SmsAccount;


typedef std::vector<std::string> vartemp_elements_vec_t;

enum{
    AUTO_TEMPLATE_TYPE_FIXED    = 0,//固定模板
    AUTO_TEMPLATE_TYPE_VARIABLE = 1,//变量模板
};

enum{
    AUTO_TEMPLATE_LEVEL_GLOBAL  = 0,//全局模板
    AUTO_TEMPLATE_LEVEL_CLIENT  = 1,//客户模板
};


struct StTemplateInfo
{
    int iTemplateId;
    int iTemplateLevel;
    int iTemplateType;
    // 以下二者取一
    vartemp_elements_vec_t varTempElements;    // 变量模板，包含元素
    std::string fixedTempContent;             // 固定模板
    bool bHasLastSeperator;
    bool bHasHeaderSeperator;

    // 当放入std::set集中，需要实现该函数
    bool operator<(const StTemplateInfo& other) const {
        // 按 template_level -> template_type -> fixedTempContent/varTempElements 顺序排序
        if (iTemplateLevel != other.iTemplateLevel)
        {
            return iTemplateType < other.iTemplateType;
        }

        if (iTemplateType != other.iTemplateType)
        {
            return iTemplateType < other.iTemplateType;
        }

        // 到这说明iTemplateType与iTemplateLevel相同

        if (iTemplateType == AUTO_TEMPLATE_TYPE_FIXED)
        {
            return fixedTempContent < other.fixedTempContent;
        }
        else if (iTemplateType == AUTO_TEMPLATE_TYPE_VARIABLE)
        {
            // 变量数目不一样，则按变量数目排序
            if (varTempElements.size() != other.varTempElements.size())
            {
                return (varTempElements.size() < other.varTempElements.size());
            }

            for (size_t i = 0; i < varTempElements.size(); ++i)
            {
                // 某元素不一样，则按该元素排序
                if (varTempElements[i] != other.varTempElements[i])
                {
                    return (varTempElements[i] < other.varTempElements[i]);
                }
            }

            // 到这则说明所有varTempElements元素也相同，则'<'比较为false
            return false;
        }

        // 执行到这，则iTemplateType都为非FIXED/VARIABLE，其实是非法的，不过当作相同吧
        return false;
    }
};

//变量模板
typedef std::list<StTemplateInfo> vartemp_list_t;
typedef std::map<std::string, vartemp_list_t> vartemp_to_user_map_t;

//固定模板
typedef std::set<StTemplateInfo> fixedtemp_set_t;
typedef std::map<std::string, fixedtemp_set_t> fixedtemp_to_user_map_t;

typedef enum
{
    STRATEGY_FORCE              = 0,    //强制路由
    STRATEGY_INTELLIGENCE,              //智能路由
    STRATEGY_TEST,                      //测试路由
    STRATEGY_WHITE_KEYWORD,             //白关键强制路由
    STRATEGY_AUTO,                      //自动路由
    STRATEGY_RESEND,                    //失败重发
    STRATEGY_SEND_LIMIT,                //发送控制路由
}ROUTE_STRATEGY;


#endif    //__C_PROTOCOL_H__
