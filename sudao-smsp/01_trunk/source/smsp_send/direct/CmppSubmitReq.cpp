#include "CmppSubmitReq.h"
#include "UTFString.h"
#include<stdio.h>
#include <stdlib.h>
namespace smsp
{

    CmppSubmitReq::CmppSubmitReq()
    {
        _msgId = 0;
        _commandId=CMPP_SUBMIT;
        _phone.reserve(21);
        _msgSrc = "000000";
        _feeType = "01";
        _msgLength = 0;
        _pkTotal = 1;
        _pkNumber = 1;
        _regDelivery = 1;
        _msgLevel = 1;
        _feeUserType = 2;
        _tpPId = 0;
        _tpUdhi = 0;
        _destUsrTl = 1;
        _msgFmt = 8;
        _serviceId = "";


    }

    CmppSubmitReq::~CmppSubmitReq()
    {

    }

    UInt32 CmppSubmitReq::bodySize() const
    {
        return _msgContent.size() + 147 + CMPPBase::bodySize();
    }

    bool CmppSubmitReq::Pack(Writer& writer) const
    {
        CMPPBase::Pack(writer);
        char serviceId[11] = {0};
        snprintf(serviceId,11,"%s",_serviceId.data());
        
        char Fee_terminal_Id[22] = {0};

        char feeCode[7] = {0};
        snprintf(feeCode, 76,"%s", "0000");

        char valIdTime[18] = {0};
        char atTime[18] = {0};

        char srcID[22] = {0};
        snprintf(srcID, 22,"%s", _srcId.data());
        const char reserve[9] = {0};
        
        char destTerminalId[22] = {0};
        snprintf(destTerminalId, 22,"%s", _phone.data());

        char msgSrc[7] = {0};
        snprintf(msgSrc,7,"%s",_msgSrc.data());

        char feeType[3] = {0};
        snprintf(feeType,3,"%s",_feeType.data());

        writer(_msgId)(_pkTotal)(_pkNumber)(_regDelivery)(_msgLevel)((UInt32) 10,serviceId)
            (_feeUserType)((UInt32) 21, Fee_terminal_Id)(_tpPId)(_tpUdhi)(_msgFmt)((UInt32) 6, msgSrc)
            ((UInt32) 2, feeType)((UInt32) 6, feeCode)((UInt32) 17, valIdTime)((UInt32) 17, atTime)
            ((UInt32) 21, srcID)(_destUsrTl)((UInt32) 21, destTerminalId)(_msgLength)
            ((UInt32) _msgLength, _msgContent.data())((UInt32) 8,reserve);

        return writer;
    }

    bool CmppSubmitReq::Unpack(Reader& reader)
    {
        return false;
    }

} /* namespace smsp */
