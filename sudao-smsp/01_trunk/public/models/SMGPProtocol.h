#ifndef SMGPPROTOCOL_H_
#define SMGPPROTOCOL_H_
#include "platform.h"

#include <string>
#include <iostream>

namespace smsp
{
    const UInt64 SMGP_LOGIN         = 0x00000001;
    const UInt64 SMGP_LOGIN_RESP        = 0x80000001;
    const UInt64 SMGP_SUBMIT            = 0x00000002;
    const UInt64 SMGP_SUBMIT_RESP       = 0x80000002;

    const UInt64 SMGP_ACTIVE_TEST           = 0x00000004;
    const UInt64 SMGP_ACTIVE_TEST_RESP      = 0x80000004;

    const UInt64 SMGP_TERMINATE         = 0x00000006;
    const UInt64 SMGP_TERMINATE_RESP    = 0x80000006;

    const UInt64 SMGP_QUERY             = 0x00000007;
    const UInt64 SMGP_QUERY_RESP        = 0x80000007;


    class SMGPProtocol
    {
    public:
        SMGPProtocol();
        virtual ~SMGPProtocol();

        SMGPProtocol(const SMGPProtocol& other);

        SMGPProtocol& operator =(const SMGPProtocol& other);

    public:
        //header
        UInt32 packetLength_;
        UInt32 requestId_;
        UInt32 sequenceId_;

        // SMGP_LOGIN 8,16,1,4,1
        std::string clientId_;
        std::string authClient_;
        UInt8 loginMode_;
        UInt32 timestamp_;
        UInt8 clientVersion_;

        //SMGP_LOGIN_RESP  4,16,1
        UInt32 status_;
        std::string authServer_;
        UInt8 serverVersion_;

        //CMPP_SUBMIT 1,1,1,10,2,6,6,1,17,17,21,21,1,21,1,msglength,8
        UInt8 msgType_;
        UInt8 needReport_;
        UInt8 priority_;
        std::string  serviceId_;
        std::string feeType_;
        std::string feeCode_;
        std::string fixedFee_;
        UInt8 msgFormat_;
        std::string validTime_;
        std::string atTime_;
        std::string srcTermId_;
        std::string chargeTermId_;
        UInt8 destTermIdCount_;
        UInt8 msgLength_;
        std::string msgContent_;
        std::string reserve_;

        //SMGP_SUBMIT_RESP 10
        std::string msgId_;
        UInt32 status_;

        //SMGP_QUERY 8,1,10
        std::string queryTime_;
        UInt8 queryType_;
        std::string queryCode_;


        //SMGP_QUERY_RESP 8,1,10
        std::string  queryTime_;
        UInt8 queryType_;
        std::string queryCode_;
        UInt32 mtTLMsg_;
        UInt32 mtTlusr_;
        UInt32 mtScs_;
        UInt32 mtWT_;
        UInt32 mtFL_;
        UInt32 moScs_;
        UInt32 moWT_;
        UInt32 moFL_;
        std::string  reserve_; //8


    };
}

#endif

