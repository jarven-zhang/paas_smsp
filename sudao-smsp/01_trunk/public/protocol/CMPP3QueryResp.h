#ifndef CMPP3QUERYRESPH
#define CMPP3QUERYRESPH
#include "CMPP3Base.h"
namespace cmpp3
{

    class CMPPQueryResp: public CMPPBase
    {
    public:
        CMPPQueryResp();
        virtual ~CMPPQueryResp();
    public:
		virtual UInt32 bodySize() const;
		virtual bool Pack(Writer &writer) const;
        virtual bool Unpack(Reader &reader);
    public:
        //CMPPQUERYRESP  8,1,10,4,4,4,4,4,4,4,4
        std::string _time;
        UInt8 _queryType;
        std::string _queryCode;
        UInt32 _mtTLMsg;
        UInt32 _mtTlusr;
        UInt32 _mtScs;
        UInt32 _mtWT;
        UInt32 _mtFL;
        UInt32 _moScs;
        UInt32 _moWT;
        UInt32 _moFL;
    };

} /* namespace smsp */
#endif /* CMPPQUERYRESPH */
