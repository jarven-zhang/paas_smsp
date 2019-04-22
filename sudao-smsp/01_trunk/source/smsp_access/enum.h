#ifndef SOURCE_SMSP_ACCESS_ENUM_H
#define SOURCE_SMSP_ACCESS_ENUM_H


typedef enum SmartTemplateMatchResult
{
    NoMatch,                        // 未匹配到模板
    MatchConstant,                  // 匹配到白模板，且是固定模板
    MatchVariable_NoMatchRegex,     // 匹配到白模板，且是变量模板，变量部分未匹配到正则
    MatchVariable_MatchRegex,       // 匹配上白模板，且是变量模板，变量部分匹配到正则

    MatchBlack,                     // 匹配到黑模板
    MatchKeyWord,                   // 匹配到关键字
}SmartTemplateMatchResult;

typedef enum OverRateResult
{
    OverRateNone       = 0,  // 未检测到超频
    OverRateGlobal     = 1,  // 全局超频
    OverRateKeyword    = 2,  // 关键字超频
    OverRateSmsType    = 3,  // 短信类型超频
    OverRateChannel    = 4,  // 通道超频
}OverRateResult;

#endif // SOURCE_SMSP_ACCESS_ENUM_H