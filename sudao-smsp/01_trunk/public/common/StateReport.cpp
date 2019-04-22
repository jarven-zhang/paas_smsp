#include "StateReport.h"

namespace smsp
{
    StateReport::StateReport()
    {
        _status = 0;
        _reportTime = 0;
		_sequenceId = 0;
		_linkId = 0;
    }

    StateReport::StateReport(const StateReport& state)
    {
        _smsId = state._smsId;
        _status = state._status;
        _reportTime = state._reportTime;
        _desc = state._desc;
		_sequenceId = state._sequenceId;
		_linkId = state._linkId;
		_clientId = state._clientId;
		_phone = state._phone;
    }

    StateReport& StateReport::operator =(const StateReport& state)
    {
        _smsId = state._smsId;
        _status = state._status;
        _reportTime = state._reportTime;
        _desc = state._desc;
		_sequenceId = state._sequenceId;
		_linkId = state._linkId;
		_clientId = state._clientId;
		_phone = state._phone;
        return *this;
    }
} /* namespace smsp */

