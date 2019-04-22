#ifndef CMPPQUERYREQ_H_
#define CMPPQUERYREQ_H_
#include "CMPPBase.h"
namespace smsp
{

    class CMPPQueryReq : public CMPPBase
    {
    public:
        CMPPQueryReq();
        virtual ~CMPPQueryReq();
    public:
        virtual UInt32 bodySize() const;
        virtual bool Pack(Writer &writer) const;
        virtual bool Unpack(Reader &reader);

    };

} /* namespace smsp */
#endif /* CMPPQUERYREQ_H_ */
