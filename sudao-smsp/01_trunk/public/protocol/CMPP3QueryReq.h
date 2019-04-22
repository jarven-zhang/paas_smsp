#ifndef CMPP3QUERYREQ_H_
#define CMPP3QUERYREQ_H_
#include "CMPP3Base.h"
namespace cmpp3
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
		std::string  _time;
		UInt8        _queryType;
		std::string  _queryCode;
		std::string  _reserve;
    };

} /* namespace smsp */
#endif /* CMPPQUERYREQ_H_ */
