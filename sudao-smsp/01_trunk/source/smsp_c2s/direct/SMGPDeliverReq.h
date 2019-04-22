#ifndef SMGPDELIVERREQ_H_
#define SMGPDELIVERREQ_H_
#include "SMGPBase.h"

namespace smsp
{
	class SMGPDeliverReq: public SMGPBase
	{
	public:
		SMGPDeliverReq();
		virtual ~SMGPDeliverReq();
	public:
		void setContent(char* content,UInt32 len)
        {
            _msgContent.reserve(20480);
            _msgContent.append(content,len);
            _msgLength = len;
        }
		virtual bool Unpack(Reader &reader);
		virtual bool Pack(Writer &writer) const;
		virtual UInt32 bodySize() const;
	public:
		// 10 ,1,1,14,21,21,1, len 8
		std::string _msgId;
		UInt64 m_uSubmitId;
		
		UInt8 _isReport;
		UInt8 _msgFmt;
		std::string _recvTime;
		std::string _srcTermId;
		std::string _destTermId;
		
		UInt8 _msgLength;
		std::string _msgContent;
		std::string _reserve;
		UInt32 _status;////smsp pin tai zhuang tai
		std::string _remark;///zhuang tai bao gao zhuang tai
		std::string _err;////zhuang tai bao gao miao shu
	};
}
#endif /* SMGPDELIVERREQ_H_ */
