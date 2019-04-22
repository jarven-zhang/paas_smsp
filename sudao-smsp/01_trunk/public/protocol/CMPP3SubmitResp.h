#ifndef CMPP3SUBMITRESP_H_
#define CMPP3SUBMITRESP_H_
#include "CMPP3Base.h"
namespace cmpp3
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
        UInt32 _result;
    };

} /* namespace smsp */
#endif /* CMPPSUBMITRESP_H_ */
