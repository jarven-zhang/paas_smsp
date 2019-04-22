#ifndef COMMONMESSAGE_H
#define COMMONMESSAGE_H

#include "Thread.h"
#include "LogMacro.h"

#include <string>

using std::string;

template<typename T>
class MiddleMsg : public TMsg
{
public:
    MiddleMsg() : m_pMsg(new T()) {}
    ~MiddleMsg() {}

private:
    MiddleMsg(const MiddleMsg&);
    MiddleMsg& operator==(const MiddleMsg&);

public:
    T* m_pMsg;
};

template<typename T>
class CommonMsg : public TMsg
{
public:
    CommonMsg() {}
    ~CommonMsg() {}

private:
    CommonMsg(const CommonMsg&);
    CommonMsg& operator==(const CommonMsg&);

public:
    T m_value;
};


class MqPublishReqMsg : public TMsg
{
public:
    MqPublishReqMsg()
        : m_iPriority(0),
          m_uiMiddlewareType(INVALID_UINT32) {}
    ~MqPublishReqMsg() {}

private:
    MqPublishReqMsg(const MqPublishReqMsg&);
    MqPublishReqMsg& operator==(const MqPublishReqMsg&);

public:
    string m_strExchange;
    string m_strRoutingKey;
    string m_strData;
    string m_strKey;    // DB消息按ID哈希
    int m_iPriority;
    UInt32 m_uiMiddlewareType;
};

class MqConsumeReqMsg : public TMsg
{
public:
    MqConsumeReqMsg()
        : m_uiType(0) {}
    ~MqConsumeReqMsg() {}

private:
    MqConsumeReqMsg(const MqConsumeReqMsg&);
    MqConsumeReqMsg& operator==(const MqConsumeReqMsg&);

public:
    UInt32 m_uiType;
    string m_strData;
};


#endif // COMMONMESSAGE_H
