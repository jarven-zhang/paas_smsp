#include "UpstreamSms.h"

namespace smsp
{
    UpstreamSms::UpstreamSms()
    {
        _upsmsTimeInt = 0;
		_sequenceId   = 0;
    }

    UpstreamSms::UpstreamSms(const UpstreamSms& upsms)
    {
        _phone     = upsms._phone;
        _appendid  = upsms._appendid;
        //_channelid = upsms._channelid;
        _upsmsTimeInt = upsms._upsmsTimeInt;
        _upsmsTimeStr = upsms._upsmsTimeStr;
        _content    = upsms._content;
		_sequenceId = upsms._sequenceId;
    }

    UpstreamSms& UpstreamSms::operator =(const UpstreamSms& upsms)
    {
        _phone     = upsms._phone;
        _appendid  = upsms._appendid;
        //_channelid = upsms._channelid;
        _upsmsTimeInt = upsms._upsmsTimeInt;
        _upsmsTimeStr = upsms._upsmsTimeStr;
        _content    = upsms._content;
		_sequenceId = upsms._sequenceId;

        return *this;
    }
} /* namespace smsp */

