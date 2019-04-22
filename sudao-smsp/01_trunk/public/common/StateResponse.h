#ifndef STATERESPONSE_H_
#define STATERESPONSE_H_
#include "platform.h"
#include <string>
#include <map>

using namespace std;
namespace smsp
{	
    class StateResponse
    {
    public:
        StateResponse();
        StateResponse( const StateResponse& state );
        StateResponse& operator=(const StateResponse& state);

	public:
        string _smsId;
		string _phone;		
        Int32   _status;
		string _desc; /* ¥ÌŒÛ√Ë ˆ*/
    };
	
	typedef std::map<std::string, StateResponse > mapStateResponse;

} /* namespace smsp */
#endif /* STATEREPORT_H_ */
