#ifndef SMPPCONNECTREQ_H_
#define SMPPCONNECTREQ_H_
#include "SMPPBase.h"
namespace smsp
{

    class SMPPConnectReq: public SMPPBase
    {
    public:
        SMPPConnectReq();
        virtual ~SMPPConnectReq();
        SMPPConnectReq(const SMPPConnectReq& other)
        {
            _systemId = other._systemId;
            _password = other._password;
            _systemType = other._systemType;

            _version = other._version;
            _addrTon = other._addrTon;
            _addrNpi = other._addrNpi;
            _addrRange = other._addrRange;
        }

        SMPPConnectReq& operator =(const SMPPConnectReq& other)
        {
            _systemId = other._systemId;
            _password = other._password;
            _systemType = other._systemType;

            _version = other._version;
            _addrTon = other._addrTon;
            _addrNpi = other._addrNpi;
            _addrRange = other._addrRange;
            return *this;
        }
    public:
        void setAccout(std::string cliendID,std::string password)
        {
            _systemId=cliendID;
            _password=password;
        }
        virtual UInt32 bodySize() const;
        virtual bool Pack(pack::Writer &writer) const;
        virtual bool Unpack(pack::Reader &reader);

    private:
// 16,9,13,1,1,1,41
        std::string _systemId;
        std::string _password;
        std::string _systemType;
        UInt8 _version;
        UInt8 _addrTon;
        UInt8 _addrNpi;
        std::string _addrRange;

    };

} /* namespace smsp */
#endif /* SMPPCONNECTREQ_H_ */
