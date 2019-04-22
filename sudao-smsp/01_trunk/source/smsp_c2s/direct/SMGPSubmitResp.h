#ifndef SMGPSUBMITRESP_H_
#define SMGPSUBMITRESP_H_
#include "SMGPBase.h"

namespace smsp
{
	class SMGPSubmitResp:public SMGPBase
	{
	public:
		SMGPSubmitResp();
		virtual ~SMGPSubmitResp();
	public:
		virtual bool Unpack(Reader &reader);
		virtual UInt32 bodySize() const;
		bool Pack(Writer &writer) const;
	public:
		std::string msgId_;
		UInt32 status_;

		UInt64 m_uSubmitId;

	};
}
#endif /* SMGPSUBMITRESP_H_ */
