#include "SMPPSubmitReq.h"
#include "UTFString.h"
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
namespace smsp
{

    SMPPSubmitReq::SMPPSubmitReq()
    {

        _commandId=SUBMIT_SM;
        _serviceType = "";
        _sourceAddrTon = 0;
        _sourceAddrNpi = 0;
        _sourceAddr = "";

        _destAddrTon = 0;
        _destAddrNpi = 0;
        _destinationAddr = "";

        _esmClass = 0;
        _protocolId = 0;
        _priorityFlag = 0;
        _scheduleDeliveryTime = "";
        _validityPeriod = "";
        _registeredDelivery = 1;
        _replaceIfPresentFlag = 0;
        _dataCoding = 8;
        _smDefaultMsgId = 0;

        _smLength = 0;
        _shortMessage = "";

    }

    SMPPSubmitReq::~SMPPSubmitReq()
    {

    }

    UInt32 SMPPSubmitReq::bodySize() const
    {
        return _shortMessage.size() +
               (_serviceType.length()<6?(_serviceType.length()+1):(_serviceType.length())) +
               (_sourceAddr.length()<21?(_sourceAddr.length()+1):(_sourceAddr.length())) +
               (_phone.length()<21?(_phone.length()+1):(_phone.length())) +
               (_scheduleDeliveryTime.length()<17?(_scheduleDeliveryTime.length()+1):(_scheduleDeliveryTime.length())) +
               (_validityPeriod.length()<17?(_validityPeriod.length()+1):(_validityPeriod.length())) +
               12 + SMPPBase::bodySize();
    }

    bool SMPPSubmitReq::Pack(pack::Writer& writer) const
    {

        char serviceType[16] = { 0 };
        char sourceAddr[32] = { 0 };
        char destinationAddr[32] = { 0 };
        char scheduleDeliveryTime[32] = { 0 };
        char validityPeriod[32] = { 0 };
        char msg[2500] = {0};
        snprintf(serviceType, 16,"%s", _serviceType.data());
        snprintf(sourceAddr, 32,"%s", _sourceAddr.data());
        snprintf(destinationAddr, 32,"%s", _phone.data());
        snprintf(scheduleDeliveryTime, 32,"%s", _scheduleDeliveryTime.data());
        snprintf(validityPeriod, 32,"%s", _validityPeriod.data());
        //sprintf(msg, "%s", _shortMessage.append());
        const char *data = _shortMessage.data();
        memcpy(msg,data,_smLength);
        LogDebug("_phone.size[%d],_phone[%s]",_phone.size(),_phone.data());
        LogDebug("_shortMessage.size[%d],_shortMessage[%s]",_shortMessage.size(),_shortMessage.data());
        LogDebug("_sourceAddr.size[%d],_sourceAddr[%s]",_sourceAddr.length(),_sourceAddr.data());
        SMPPBase::Pack(writer);
        writer((UInt32)(_serviceType.length()<6?(_serviceType.length()+1):(_serviceType.length())), serviceType)
        (_sourceAddrTon)(_sourceAddrNpi)((UInt32)(_sourceAddr.length()<21?(_sourceAddr.length()+1):(_sourceAddr.length())),sourceAddr)
        (_destAddrTon)(_destAddrNpi)((UInt32)(_phone.length()<21?(_phone.length()+1):(_phone.length())),destinationAddr)(_esmClass)
        (_protocolId)(_priorityFlag)((UInt32) (_scheduleDeliveryTime.length()<17?(_scheduleDeliveryTime.length()+1):(_scheduleDeliveryTime.length())),scheduleDeliveryTime)
        ((UInt32) (_validityPeriod.length()<17?(_validityPeriod.length()+1):(_validityPeriod.length())),validityPeriod)
        (_registeredDelivery)(_replaceIfPresentFlag)(_dataCoding)(_smDefaultMsgId)(_smLength)
        ((UInt32) _smLength, msg);

        return writer;
    }

    bool SMPPSubmitReq::Unpack(pack::Reader& reader)
    {
        return false;
    }

} /* namespace smsp */
