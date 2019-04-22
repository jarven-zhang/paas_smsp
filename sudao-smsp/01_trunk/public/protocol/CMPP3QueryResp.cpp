#include "CMPP3QueryResp.h"
#include "CMPP3QueryReq.h"
namespace cmpp3
{

    CMPPQueryResp::CMPPQueryResp()
    {
        _queryType = 0;
		_queryCode = "";
        _mtTLMsg = 0;
        _mtTlusr = 0;
        _mtScs = 0;
        _mtWT = 0;
        _mtFL = 0;
        _moScs = 0;
        _moWT = 0;
        _moFL = 0;
		_commandId = CMPP_QUERY_RESP;
    }

    CMPPQueryResp::~CMPPQueryResp()
    {

    }
	UInt32 CMPPQueryResp::bodySize() const
    {
        return 51 + CMPPBase::bodySize();
    }
	bool CMPPQueryResp::Pack(Writer& writer) const
	{
		char time[80];
        timestampYear(time);
		CMPPBase::Pack(writer);
		writer((UInt32) 8, time)(_queryType)((UInt32) 10, _queryCode.data())(_mtTLMsg)
			(_mtTlusr)(_mtScs)(_mtWT)(_mtFL)(_moScs)(_moWT)(_moFL);\
        return writer;
	}
    bool CMPPQueryResp::Unpack(Reader& reader)
    {
        reader((UInt32) 8, _time)(_queryType)((UInt32) 10, _queryCode)(_mtTLMsg)(
            _mtTlusr)(_mtScs)(_mtWT)(_mtFL)(_moScs)(_moWT)(_moFL);
        printf("time:[%s]\n",_time.data());
        printf("_queryType:[%d]\n",_queryType);
        printf("_queryCode:[%s]\n",_queryCode.data());
        printf("_mtTLMsg:[%d]\n",_mtTLMsg);
        printf("_mtTlusr:[%d]\n",_mtTlusr);
        printf("_mtScs:[%d]\n",_mtScs);
        printf("_mtWT:[%d]\n",_mtWT);
        printf("_mtFL:[%d]\n",_mtFL);
        printf("_moScs:[%d]\n",_moScs);
        printf("_moWT:[%d]\n",_moWT);
        printf("_moFL:[%d]\n",_moFL);
        return reader;
    }

} /* namespace smsp */
