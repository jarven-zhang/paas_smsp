#ifndef __COMPONENTREFMQ__
#define __COMPONENTREFMQ__

#include "platform.h"

#include <string>
#include <map>

using std::string;
using std::map;

const string MESSAGE_TYPE_DB             = "00";
const string MESSAGE_TYPE_YD_HY          = "01";
const string MESSAGE_TYPE_YD_YX          = "02";
const string MESSAGE_TYPE_LD_HY          = "03";
const string MESSAGE_TYPE_LD_YX          = "04";
const string MESSAGE_TYPE_DX_HY          = "05";
const string MESSAGE_TYPE_DX_YX          = "06";
const string MESSAGE_TYPE_HY             = "07";
const string MESSAGE_TYPE_YX             = "08";
const string MESSAGE_TYPE_ABNORMAL_YD_HY = "11";
const string MESSAGE_TYPE_ABNORMAL_YD_YX = "12";
const string MESSAGE_TYPE_ABNORMAL_LT_HY = "13";
const string MESSAGE_TYPE_ABNORMAL_LT_YX = "14";
const string MESSAGE_TYPE_ABNORMAL_DX_HY = "15";
const string MESSAGE_TYPE_ABNORMAL_DX_YX = "16";

const string MESSAGE_TYPE_MO             = "21";//上行消息
const string MESSAGE_TYPE_REPORT         = "22";//状态报告缓存消息

const string MESSAGE_AUDIT_CONTENT_REQ           = "23";
const string MESSAGE_CONTENT_AUDIT_RESULT        = "24";

const string MESSAGE_TYPE_MONITOR_C2S_HTTP     =  "30"; //smsp_c2s/smsp_http监控消息，
const string MESSAGE_TYPE_MONITOR_ACCESS       =  "31"; //smsp_access监控消息，
const string MESSAGE_TYPE_MONITOR_SEND_REPORT  =  "32"; //smsp_send/smsp_report监控消息，
const string MESSAGE_TYPE_MONITOR_REBACK       =  "33"; //smsp_reback监控消息，
const string MESSAGE_TYPE_MONITOR_CHARGE       =  "34"; //smsp_charge监控消息，
const string MESSAGE_TYPE_MONITOR_AUDIT        =  "35"; //smsp_audit监控消息，
const string MESSAGE_TYPE_MONITOR_MID          =  "36"; //中间件监控消息

const UInt32 PRODUCT_MODE  = 0;
const UInt32 CONSUMER_MODE = 1;

class ComponentRefMq
{
public:
    ComponentRefMq();
    ~ComponentRefMq();

public:
    UInt64  m_uId;
    UInt64  m_uComponentId;
    string  m_strMessageType;
    UInt64  m_uMqId;
    UInt32  m_uGetRate;
    UInt32  m_uMode;
    string  m_strRemark;
    UInt32  m_uWeight;
};

typedef map<string, ComponentRefMq> ComponentRefMqMap;
typedef ComponentRefMqMap::iterator ComponentRefMqMapIter;

#endif ////__COMPONENTREFMQ__

