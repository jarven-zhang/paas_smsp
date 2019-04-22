#ifndef STATEREPORT_H_
#define STATEREPORT_H_
#include "platform.h"
#include <string>
using namespace std;
namespace smsp
{

    class StateReport
    {
    public:
        StateReport();
        StateReport(const StateReport& state);
        StateReport& operator=(const StateReport& state);
        string _smsId;
        Int32   _status;
        Int64 _reportTime;
        string _desc;
		UInt32 _sequenceId;
		Int32 _linkId;
		string _clientId;
		string _phone;
    };

} /* namespace smsp */
#endif /* STATEREPORT_H_ */
