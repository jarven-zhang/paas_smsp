#ifndef CALLBACK_H_
#define CALLBACK_H_
#include "platform.h"

#include <string>
using namespace std;
namespace models
{

    class Callback
    {
    public:
        Callback();
        virtual ~Callback();
        Callback(const Callback& other)
        {
            _callbackUrl=other._callbackUrl;
            _voiceNotifyUrl=other._voiceNotifyUrl;
            _index=other._index;
            _smsId=other._smsId;
            _sign=other._sign;
            m_strSid=other.m_strSid;
            _reportflag = other._reportflag;
            _restSmsId = other._restSmsId;
        }

        Callback& operator=(const Callback& other)
        {
            _callbackUrl=other._callbackUrl;
            _voiceNotifyUrl=other._voiceNotifyUrl;
            _index=other._index;
            _smsId=other._smsId;
            _sign=other._sign;
            m_strSid=other.m_strSid;
            _reportflag = other._reportflag;
            _restSmsId = other._restSmsId;
            return *this;
        }
    public:
        UInt32 _index;
        string _smsId;
        string _restSmsId;
        string _callbackUrl;
        string _voiceNotifyUrl;
        string _sign;
        string m_strSid;
        Int32 _reportflag;
    };

} /* namespace models */
#endif /* CALLBACK_H_ */
