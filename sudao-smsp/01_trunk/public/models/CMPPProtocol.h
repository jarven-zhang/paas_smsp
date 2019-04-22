#ifndef CMPPPROTOCOL_H_
#define CMPPPROTOCOL_H_
#include <string>
#include <iostream>
#include "platform.h"


namespace smsp
{
    const UInt64 CMPP_CONNECT           = 0x00000001;
    const UInt64 CMPP_CONNECT_RESP      = 0x80000001;
    const UInt64 CMPP_TERMINATE         = 0x00000002;
    const UInt64 CMPP_TERMINATE_RESP    = 0x80000002;
    const UInt64 CMPP_SUBMIT            = 0x00000004;
    const UInt64 CMPP_SUBMIT_RESP       = 0x80000004;

    const UInt64 CMPP_QUERY             = 0x00000006;
    const UInt64 CMPP_QUERY_RESP        = 0x80000006;

    const UInt64 CMPP_ACTIVE_TEST       = 0x00000008;
    const UInt64 CMPP_ACTIVE_TEST_RESP  = 0x80000008;


    class CMPPProtocol
    {
    public:
        CMPPProtocol();
        virtual ~CMPPProtocol();

        CMPPProtocol(const CMPPProtocol& other);

        CMPPProtocol& operator =(const CMPPProtocol& other);

    public:
        //header
        UInt32 totalLength_;
        UInt32 commandId_;
        UInt32 sequenceId_;

        //connect CMPP_CONNECT 6,16,1,4
        std::string sourceAddr_;
        std::string authSource_;
        UInt8 version_;
        UInt32 timestamp;

        //CMPP_CONNECT_RESP 1,16,1
        UInt8 status_;
        std::string authISMG_;
        UInt8 version_;

        //CMPP_SUBMIT 8,1,1,1,1,10,1,21,1,1,1,6,2,6,17,17,21,1,21,1,msglength,8
        UInt8 msgId_;
        UInt8 pkTotal_;
        UInt8 pkNumber_;
        UInt8 regDelivery_;
        UInt8 msgLevel_;
        std::string serviceId_;
        UInt8 feeUserType_;
        UInt64 feeTerminalId_;
        UInt8 tpPId_;
        UInt8 tpUdhi_;
        UInt8 msgFmt_;
        std::string msgSrc_;
        std::string feeType_;
        std::string feeCode_;
        std::string valIdTime_;
        std::string atTime_;
        std::string srcId_;
        UInt8 destUsrTl_;
        std::string destTerminalId_;
        UInt8 msgLength_;
        std::string msgContent_;
        std::string reserve_;

        //CMPP_SUBMIT_RESP 8,1
        UInt64 submit_msgId_;
        UInt8 result_;

        //CMPP_QUERY 8,1,10,8
        std::string time_;
        UInt8 queryType_;
        std::string queryCode_;
        std::string  reserve_;

        //CMPP_QUERY_RESP  8,1,10,4,4,4,4,4,4,4,4
        std::string  time_;
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



    };

}

#endif

