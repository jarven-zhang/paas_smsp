#ifndef CMPPDELIVERRESP_H_
#define CMPPDELIVERRESP_H_
#include "CMPPBase.h"
namespace smsp
{

    class CMPPDeliverResp : public CMPPBase
    {
    public:
        CMPPDeliverResp();
        virtual ~CMPPDeliverResp();
    public:
        virtual UInt32 bodySize() const;
        virtual bool Pack(Writer &writer) const;
		virtual bool Unpack(Reader& reader);
    public:
        UInt64 _msgId;
        UInt8 _result;
    };

} /* namespace smsp */
#endif /* CMPPDELIVERRESP_H_ */
