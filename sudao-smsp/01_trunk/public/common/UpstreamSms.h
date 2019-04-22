#ifndef UPSTREAMSMS_H_
#define UPSTREAMSMS_H_
#include "platform.h"
#include <string>
using namespace std;
namespace smsp
{

    class UpstreamSms
    {
    public:
        UpstreamSms();
        UpstreamSms(const UpstreamSms& upsms);
        UpstreamSms& operator=(const UpstreamSms& upsms);
        string  _phone;
        string  _appendid;
        //string    _channelid;
        string _upsmsTimeStr;
        UInt64 _upsmsTimeInt;
        string _content;
		UInt32 _sequenceId;
    };


} /* namespace smsp */
#endif /* UPSTREAMSMS_H_ */
