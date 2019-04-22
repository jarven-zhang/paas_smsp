#ifndef CMPPDELIVERREQ_H_
#define CMPPDELIVERREQ_H_
#include "CMPPBase.h"
namespace smsp
{

    class CMPPDeliverReq : public CMPPBase
    {
    public:
        CMPPDeliverReq();
        virtual ~CMPPDeliverReq();
    public:
        virtual UInt32 bodySize() const;
        virtual bool Pack(Writer &writer) const;
    public:
        UInt64 _msgId;
        UInt8 _result;
    };

} /* namespace smsp */
#endif /* CMPPDELIVERREQ_H_ */
