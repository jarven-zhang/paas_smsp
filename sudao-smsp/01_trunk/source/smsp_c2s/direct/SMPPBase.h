#ifndef SMPPBASE_H_
#define SMPPBASE_H_
#include "platform.h"
#include <iostream>
#include <string.h>

#include "readerEx.h"
#include "writerEx.h"
#include <stdio.h>
#include <vector>
#include "CmdCode.h"


using namespace std;

typedef enum
{
    SMPP_OK                        = 0x00000000,     // No error
    SMPP_INVMSGLEN                 = 0x00000001,     // Message Length is invalid
    SMPP_INVCMDLEN                 = 0x00000002,     // Command Length is invalid
    SMPP_INVCMDID                  = 0x00000003,     // Invalid Command ID
    SMPP_INVBNDSTS                 = 0x00000004,     // Incorrect BIND Status for given command
    SMPP_ALYBND                    = 0x00000005,     // SMPP Already in bound state
    SMPP_INVPRTFLG                 = 0x00000006,     // Invalid priority flag
    SMPP_INVREGDLVFLG              = 0x00000007,     // Invalid registered delivery flag
    SMPP_SYSERR                    = 0x00000008,     // System Error
    SMPP_INVSRCADR                 = 0x0000000a,     // Invalid source address
    SMPP_INVDSTADR                 = 0x0000000b,     // Invalid destination address
    SMPP_INVMSGID                  = 0x0000000c,     // Message ID is invalid
    SMPP_BINDFAIL                  = 0x0000000d,     // Bind failed
    SMPP_INVPASWD                  = 0x0000000e,     // Invalid password
    SMPP_INVSYSID                  = 0x0000000f,     // Invalid System ID
    SMPP_CANCELFAIL                = 0x00000011,     // Cancel SM Failed
    SMPP_REPLACEFAIL               = 0x00000013,     // Replace SM Failed
    SMPP_MSGQFUL                   = 0x00000014,     // Message queue full
    SMPP_INVSERTYP                 = 0x00000015,     // Invalid service type
    SMPP_INVNUMDESTS               = 0x00000033,     // Invalid number of destinations
    SMPP_INVDLNAME                 = 0x00000034,     // Invalid distribution list name
    SMPP_INVDESTFLAG               = 0x00000040,     // Destination flag is invalid (submit_multi)
    SMPP_INVSUBREP                 = 0x00000042,     //  request (i.e. submit_sm with replace_if_present_flag set)"},
    SMPP_INVESMCLASS               = 0x00000043,     // Invalid esm_class field data
    SMPP_CNTSUBDL                  = 0x00000044,     // Cannot submit to distribution list
    SMPP_SUBMITFAIL                = 0x00000045,     // submit_sm or submit_multi failed
    SMPP_INVSRCTON                 = 0x00000048,     // Invalid source address TON
    SMPP_INVSRCNPI                 = 0x00000049,     // Invalid source address NPI
    SMPP_INVDSTTON                 = 0x00000050,     // Invalid destination address TON
    SMPP_INVDSTNPI                 = 0x00000051,     // Invalid destination address NPI
    SMPP_INVSYSTYP                 = 0x00000053,     // Invalid system_type field
    SMPP_INVREPFLAG                = 0x00000054,     // Invalid replace_if_present flag
    SMPP_INVNUMMSGS                = 0x00000055,     // Invalid number of messages
    SMPP_THROTTLED                 = 0x00000058,     // Throttling error (SMPP has exceeded allowed message limits)
    SMPP_INVSCHED                  = 0x00000061,     // Invalid scheduled delivery time
    SMPP_INVEXPIRY                 = 0x00000062,     // Invalid message validity period (expiry time)
    SMPP_INVDFTMSGID               = 0x00000063,     // Predefined message invalid or not found
    SMPP_X_T_APPN                  = 0x00000064,     // SMPP Receiver Temporary App Error Code
    SMPP_X_P_APPN                  = 0x00000065,     // SMPP Receiver Permanent App Error Code
    SMPP_X_R_APPN                  = 0x00000066,     // SMPP Receiver Reject Message Error Code
    SMPP_QUERYFAIL                 = 0x00000067,     // query_sm request failed
    SMPP_INVOPTPARSTREAM           = 0x000000c0,     // Error in the optional part of the PDU Body
    SMPP_OPTPARNOTALLWD            = 0x000000c1,     // Optional paramenter not allowed
    SMPP_INVPARLEN                 = 0x000000c2,     // Invalid parameter length
    SMPP_MISSINGOPTPARAM           = 0x000000c3,     // Expected optional parameter missing
    SMPP_INVOPTPARAMVAL            = 0x000000c4,     // Invalid optional parameter value
    SMPP_DELIVERYFAILURE           = 0x000000fe,     // Delivery Failure (used for data_sm_resp)
    SMPP_UNKNOWNERR                = 0x000000ff,     // Unknown error
}SMPP_ERROR_CODE;


class SMPPBase: public pack::Packet
{
public:
    SMPPBase();
    virtual ~SMPPBase();
    SMPPBase(const SMPPBase& other)
    {
        _commandLength = other._commandLength;
        _commandId = other._commandId;
        _commandStatus = other._commandStatus;
        _sequenceNum = other._sequenceNum;
    }

    SMPPBase& operator =(const SMPPBase& other)
    {
        _commandLength = other._commandLength;
        _commandId = other._commandId;
        _commandStatus = other._commandStatus;
        _sequenceNum = other._sequenceNum;

        return *this;
    }
    const char* getCode(UInt32 code)const;
public:

    virtual UInt32 bodySize() const
    {
        return 16;
    }
    virtual bool Pack(pack::Writer &writer) const;
    virtual bool Unpack(pack::Reader &reader);

    virtual std::string Dump() const
    {
        return "";
    }
    static UInt32 getSeqID();
    static UInt32 g_seqId;
public:
    //header
    UInt32 _commandLength;
    UInt32 _commandId;
    UInt32 _commandStatus;
    UInt32 _sequenceNum;
private:

};

#endif /* SMPPBASE_H_ */
