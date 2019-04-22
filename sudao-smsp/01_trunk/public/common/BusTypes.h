#ifndef __BUS_TYPES_H__
#define __BUS_TYPES_H__

namespace BusType
{

#define     SMS_TYPE_NOTICE_FLAG             0x1
#define     SMS_TYPE_VERIFICATION_CODE_FLAG  0x2
#define     SMS_TYPE_MARKETING_FLAG          0x4
#define     SMS_TYPE_ALARM_FLAG              0x8
#define     SMS_TYPE_USSD_FLAG               0x10
#define     SMS_TYPE_FLUSH_SMS_FLAG          0x20

enum
{
    CLIENT_STATUS_REGISTERED = 1,  // 注册完成
    CLIENT_STATUS_FREEZED = 5,     // 冻结
    CLIENT_STATUS_LOGOFF = 6,      // 注销
    CLIENT_STATUS_LOCKED = 7,      // 锁定
};

enum
{
    SMS_FROM_ACCESS_CMPP3 = 1,
    SMS_FROM_ACCESS_SMPP = 2,
    SMS_FROM_ACCESS_CMPP = 3,
    SMS_FROM_ACCESS_SGIP = 4,
    SMS_FROM_ACCESS_SMGP = 5,
    SMS_FROM_ACCESS_HTTP = 6,
    SMS_FROM_ACCESS_HTTP_2 = 7,
    SMS_FROM_ACCESS_HTTP_3 = 8,
    SMS_FROM_ACCESS_MAX,
};

enum PRODUCT_TYPE
{
    PRODUCT_TYPE_HANG_YE = 0,
    PRODUCT_TYPE_YING_XIAO = 1,
    PRODUCT_TYPE_GOU_JI = 2,
    PRODUCT_TYPE_YAN_ZHENG_MA = 3,
    PRODUCT_TYPE_TONG_ZHI = 4,
    PRODUCT_TYPE_USSD = 7,    //USSD
    PRODUCT_TYPE_FLUSH_SMS = 8,    //闪信
    PRODUCT_TYPE_HANG_UP_SMS = 9,    //挂机短信
};

enum TemplateType
{
    TEMPLATE_TYPE_NOTICE = 0,    //通知模板
    TEMPLATE_TYPE_VERIFICATION_CODE = 4,    //验证码模板
    TEMPLATE_TYPE_MARKETING = 5,    //营销模板
    TEMPLATE_TYPE_USSD = 7,    //USSD模板
    TEMPLATE_TYPE_FLUSH_SMS = 8,    //闪信模板
    TEMPLATE_TYPE_HANG_UP_SMS = 9,    //挂机短信模板
};

enum SmsType
{
    SMS_TYPE_NOTICE = 0,    //通知短信
    SMS_TYPE_VERIFICATION_CODE = 4,    //验证码短信
    SMS_TYPE_MARKETING = 5,    //营销短信
    SMS_TYPE_ALARM = 6,    //告警短信
    SMS_TYPE_USSD = 7,    //USSD
    SMS_TYPE_FLUSH_SMS = 8,    //闪信
};

enum SendType
{
    SEND_TYPE_HY = 0, //行业
    SEND_TYPE_YX = 1,//营销
};

enum SMS_TYPE
{
    SMS_TYPE_TONG_ZHI = 0,
    SMS_TYPE_YAN_ZHENG_MA = 4,
    SMS_TYPE_YING_XIAO = 5,
};

enum
{
    SMS_TO_AUDIT = 2,   /// send to smsp_audit
    SMS_TO_CHARGE = 3,  //// sned to smsp_charge
};

//是否需要审核，0：不需要，1：营销需要，2：全部需要，3：关键字审核
enum
{
    SMS_NO_NEED_AUDIT       = 0,
    SMS_YINGXIAO_NEED_AUDIT = 1,
    SMS_ALL_NEED_AUDIT      = 2,
    SMS_KEYWORD_AUDIT       = 3,
};

enum PHONETYPE
{
    OTHER = 0,
    YIDONG = 1,
    LIANTONG = 2,
    DIANXIN = 3,
    FOREIGN = 4,
    XNYD = 5,
    XNLT = 6,
    XNDX = 7
};

enum
{
    SHOWSIGNTYPE_PRO = 1,  // 前置
    SHOWSIGNTYPE_POST = 2  // 后置
};

enum TIMERSEND_SUBMITTYPE
{
    TSST_SUBACCOUNT = 0,
    TSST_AGENT = 1
};

enum
{
    CLIENT_ASCRIPTION_ALI = 0,
    CLIENT_ASCRIPTION_AGENT = 1,
    CLIENT_ASCRIPTION_CLOUNT = 2,
};

enum
{
    AGENTTYPE_SALE = 1,
    AGENTTYPE_BRAND = 2,
    AGENTTYPE_OEM = 5,
};

}

enum PayType
{
    PayType_Pre = 0,            // 预付费
    PayType_Post = 1,           // 后付费
    PayType_ChargeAgent = 2,    // 扣主账户
    PayType_ChargeAccount = 3,  // 扣子账户
};


#endif
