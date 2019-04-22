#ifndef CMPP3DELIVERRESP_H_
#define CMPP3DELIVERRESP_H_
#include "CMPP3Base.h"
namespace cmpp3
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
        UInt32 _result;
    };

} /* namespace smsp */
#endif /* CMPPDELIVERRESP_H_ */
