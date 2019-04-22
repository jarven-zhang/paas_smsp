#ifndef SMGPCONNECTRESP_H_
#define SMGPCONNECTRESP_H_
#include "SMGPBase.h"

namespace smsp
{
	class SMGPConnectResp: public SMGPBase
	{
	public:
		enum SMGCONNSTATE
		{
			CONNSTATESUC = 0,
			CONNSTSTEFAILED
		};
		SMGPConnectResp();
		virtual ~SMGPConnectResp();
	public:
		std::string getAuthServer();
		virtual bool Pack(Writer &writer) const;
		virtual bool Unpack(Reader &reader);
		virtual UInt32 bodySize() const;
		bool isLogin()
		{
			return isOK_;
		}
	public:
		std::string password_;
		std::string authClient_;

		//SMGP_LOGIN_RESP  4,16,1
		UInt32 status_;
		std::string authServer_;
		UInt8 serverVersion_;
		

		bool isOK_;
	};
}
#endif
