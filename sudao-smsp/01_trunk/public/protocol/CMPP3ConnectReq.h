#ifndef CMPP3CONNECTREQ_H_
#define CMPP3CONNECTREQ_H_
#include "CMPP3Base.h"
namespace cmpp3
{

    class CMPPConnectReq: public CMPPBase
    {
    public:
        CMPPConnectReq();
        virtual ~CMPPConnectReq();
        CMPPConnectReq(const CMPPConnectReq& other)
        {
            _sourceAddr = other._sourceAddr;
            _authSource = other._authSource;
            _version = other._version;
            _timestamp = other._timestamp;
        }

        CMPPConnectReq& operator =(const CMPPConnectReq& other)
        {
            _sourceAddr = other._sourceAddr;
            _authSource = other._authSource;
            _version = other._version;
            _timestamp = other._timestamp;
            return *this;
        }
        void createAuthSource();
    public:
        void setAccout(std::string cliendID,std::string password)
        {
            _sourceAddr=cliendID;
            _password=password;
            createAuthSource();
        }
        virtual UInt32 bodySize() const;
        virtual bool Pack(Writer &writer) const;
        virtual bool Unpack(Reader &reader);

    public:
        std::string _password;
        //connect CMPP_CONNECT 6,16,1,4
        std::string _sourceAddr;
        std::string _authSource;
        UInt8 _version;
        UInt32 _timestamp;

    };

} /* namespace smsp */
#endif /* CMPPCONNECTREQ_H_ */
