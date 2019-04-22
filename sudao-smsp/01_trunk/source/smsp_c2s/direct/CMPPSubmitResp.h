#ifndef CMPPSUBMITRESP_H_
#define CMPPSUBMITRESP_H_
#include "CMPPBase.h"
namespace smsp
{

    class CMPPSubmitResp:public CMPPBase
    {
    public:
        CMPPSubmitResp();
        virtual ~CMPPSubmitResp();
    public:
        virtual bool Unpack(Reader &reader);
		bool Pack(Writer& writer) const;
		virtual UInt32 bodySize() const;
    public:
        UInt64 _msgID;
        UInt8 _result;
    };

} /* namespace smsp */
#endif /* CMPPSUBMITRESP_H_ */
