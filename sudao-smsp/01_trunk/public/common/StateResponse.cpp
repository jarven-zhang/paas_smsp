#include "StateResponse.h"

namespace smsp
{
    StateResponse::StateResponse()
    {
        _status = 0;
    }
	
    StateResponse::StateResponse( const StateResponse& state )
    {
        _smsId = state._smsId;
        _status = state._status;
		_phone = state._phone;
		_desc  = state._desc;
    }

    StateResponse& StateResponse::operator =(const StateResponse& state)
    {
        _smsId = state._smsId;
        _status = state._status;
		_phone = state._phone;		
		_desc  = state._desc;
        return *this;
    }
} /* namespace smsp */

