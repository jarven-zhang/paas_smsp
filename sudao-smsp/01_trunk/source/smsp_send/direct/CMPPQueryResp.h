#ifndef CMPPQUERYRESPH
#define CMPPQUERYRESPH
#include "CMPPBase.h"
namespace smsp
{

    class CMPPQueryResp: public CMPPBase
    {
    public:
        CMPPQueryResp();
        virtual ~CMPPQueryResp();
    public:
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
