#include "SMPPSubmitReq.h"
#include "UTFString.h"
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
namespace smsp
{

    SMPPSubmitReq::SMPPSubmitReq()
    {
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
        _registeredDelivery = 0;
        _replaceIfPresentFlag = 0;
        _dataCoding = 0;
        _smDefaultMsgId = 0;
        _smLength = 0;
        _shortMessage = "";
        _pkNumber = 0;
        _pkTotal = 0;
        _randeCode = 0;
        _phoneCount = 0;
        _phone = "";
        _buffer = "";

    }

    SMPPSubmitReq::~SMPPSubmitReq()
    {

    }
    
    bool SMPPSubmitReq::readCOctetString(UInt32& position, UInt32 max, char* data, bool isBreak)
    {
        const char *src = NULL;
        UInt32 size = 0;
        src = _buffer.data() + position;
        size = _buffer.size() - position;
        UInt32 i = 0;
        for(; i<max; i++)
        {
            if((src[i]==0x00) && (false == isBreak))
            {
                i++;
                break;
            }
        }
        if (i <= size)
        {
            memcpy(data, src, i);
            position += i;
            return true;
        }
        else
        {
            return false;
        }
    }
    
    bool SMPPSubmitReq::Unpack(pack::Reader& reader)
    {
        UInt32 first = (UInt32)_commandLength-bodySize();
        UInt32 position=0;

        char serviceType[10] = {0};          ////6
        char sourceAddr[25] = {0};           ////21
        char destinationAddr[25] = {0};      ////21
        char scheduleDeliveryTime[25] = {0}; //// 17
        char validityPeriod[25] = {0};       ////17
        char shortMessage[700] = {0};

        UInt8 uTemp = 0;
        
        if (reader(first, _buffer))
        {
            readCOctetString(position, 6, serviceType);
            readCOctetString(position, 1, reinterpret_cast<char*>(&_sourceAddrTon));
            readCOctetString(position, 1, reinterpret_cast<char*>(&_sourceAddrNpi));
            readCOctetString(position, 21, sourceAddr);
            readCOctetString(position, 1, reinterpret_cast<char*>(&_destAddrTon));
            readCOctetString(position, 1, reinterpret_cast<char*>(&_destAddrNpi));
            readCOctetString(position, 21, destinationAddr);
            readCOctetString(position, 1, reinterpret_cast<char*>(&_esmClass));
            readCOctetString(position, 1, reinterpret_cast<char*>(&_protocolId));
            readCOctetString(position, 1, reinterpret_cast<char*>(&_priorityFlag));
            readCOctetString(position, 17, scheduleDeliveryTime);
            readCOctetString(position, 17, validityPeriod);
            readCOctetString(position, 1, reinterpret_cast<char*>(&_registeredDelivery));
            readCOctetString(position, 1, reinterpret_cast<char*>(&_replaceIfPresentFlag));
            readCOctetString(position, 1, reinterpret_cast<char*>(&_dataCoding));
            readCOctetString(position, 1, reinterpret_cast<char*>(&_smDefaultMsgId));
            readCOctetString(position, 1, reinterpret_cast<char*>(&_smLength));
            ////LogNotice("===========smLength[%d]",_smLength);
            
            _serviceType = serviceType;
            _sourceAddr = sourceAddr;
            _destinationAddr = destinationAddr;
            _scheduleDeliveryTime = scheduleDeliveryTime;
            _validityPeriod = validityPeriod;

            ////long short message
            if (_esmClass&0x40)
            {
                readCOctetString(position, 1, reinterpret_cast<char*>(&uTemp));
                readCOctetString(position, 1, reinterpret_cast<char*>(&uTemp));
                readCOctetString(position, 1, reinterpret_cast<char*>(&uTemp));
                readCOctetString(position, 1, reinterpret_cast<char*>(&uTemp));
                _randeCode = uTemp;
                uTemp = 0;
                readCOctetString(position, 1, reinterpret_cast<char*>(&uTemp));
                _pkTotal = uTemp;
                uTemp = 0;
                readCOctetString(position, 1, reinterpret_cast<char*>(&uTemp));
                _pkNumber = uTemp;
                ////LogDebug("randeCode[%u],pkNumber[%u],pkTotal[%u].",_randeCode,_pkNumber,_pkTotal);
                
                readCOctetString(position, _smLength-6, shortMessage,true);
                _shortMessage.append(shortMessage,_smLength-6);
            }
            else  ////short message
            {
                _pkNumber = 1;
                _pkTotal = 1;
                readCOctetString(position, _smLength, shortMessage,true);
                _shortMessage.append(shortMessage,_smLength);
            }

            _phone = _destinationAddr;
            _phoneCount = 1;
            _phonelist.push_back(_destinationAddr);

            LogDebug("sourceAddr[%s],destAddr[%s],esmClass[%d],dataCode[%d],msgLength[%d],msgContent[%s],pkNumber[%d],pkTotal[%d]",
                _sourceAddr.data(),_phone.data(),_esmClass,_dataCoding,_smLength,_shortMessage.data(),_pkNumber,_pkTotal);

            ////LogDebug("_phone.size[%d],_phone[%s]",_phone.size(),_phone.data());
            ////LogDebug("_shortMessage.size[%d],_shortMessage[%s]",_shortMessage.size(),_shortMessage.data());
            ////LogDebug("_sourceAddr.size[%d],_sourceAddr[%s]",_sourceAddr.length(),_sourceAddr.data());
            ////LogDebug("smlength[%d]",_smLength);

            
        }
        else
        {
            LogWarn("smpp submit unpack read is error.");
        }
        return reader;
    }

} /* namespace smsp */
