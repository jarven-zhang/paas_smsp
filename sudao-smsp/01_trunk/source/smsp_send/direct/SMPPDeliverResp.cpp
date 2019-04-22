#include "SMPPDeliverResp.h"
#include <arpa/inet.h>
#include <string.h>
#include "main.h"
namespace smsp
{

    SMPPDeliverResp::SMPPDeliverResp()
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
        _registeredDelivery = 1;
        _replaceIfPresentFlag = 0;
        _dataCoding = 0;
        _smDefaultMsgId = 0;

        _smLength = 0;
        _shortMessage = "";
        _status=4;

    }

    SMPPDeliverResp::~SMPPDeliverResp()
    {

    }
    bool SMPPDeliverResp::readCOctetString(UInt32& position, UInt32 max, char* data)
    {
        const char *src = NULL;
        UInt32 size = 0;
        src = _buffer.data() + position;
        size = _buffer.size() - position;
        UInt32 i = 0;
        for(; i<max; i++)
        {
            if(src[i]==0x00)
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

    bool SMPPDeliverResp::Unpack(pack::Reader& reader)
    {
        ///UInt32 first=reader.GetSize();
        UInt32 first = (UInt32)_commandLength-bodySize();
        UInt32 position=0;

        char serviceType[16] = { 0 };
        char sourceAddr[32] = { 0 };
        char destinationAddr[32] = { 0 };
        char scheduleDeliveryTime[32] = { 0 };
        char validityPeriod[32] = { 0 };
        char msg[8192] = { 0 };
        reader(first, _buffer);

        readCOctetString(position, 6, serviceType);
        readCOctetString(position, 1, reinterpret_cast<char *>(&_sourceAddrTon));
        readCOctetString(position, 1, reinterpret_cast<char *>(&_sourceAddrNpi));
        readCOctetString(position, 21, sourceAddr);
        readCOctetString(position, 1, reinterpret_cast<char *>(&_destAddrTon));
        readCOctetString(position, 1, reinterpret_cast<char *>(&_destAddrNpi));
        readCOctetString(position, 21, destinationAddr);
        readCOctetString(position, 1, reinterpret_cast<char *>(&_esmClass));
        readCOctetString(position, 1, reinterpret_cast<char *>(&_protocolId));
        readCOctetString(position, 1, reinterpret_cast<char *>(&_priorityFlag));
        readCOctetString(position, 21, scheduleDeliveryTime);
        readCOctetString(position, 21, validityPeriod);
        readCOctetString(position, 1, reinterpret_cast<char *>(&_registeredDelivery));
        readCOctetString(position, 1, reinterpret_cast<char *>(&_replaceIfPresentFlag));
        readCOctetString(position, 1, reinterpret_cast<char *>(&_dataCoding));
        readCOctetString(position, 1, reinterpret_cast<char *>(&_smDefaultMsgId));
        readCOctetString(position, 1, reinterpret_cast<char *>(&_smLength));
        readCOctetString(position, _smLength, msg);

        _shortMessage = msg;
        _destinationAddr = destinationAddr;
        _sourceAddr = sourceAddr;
        LogDebug("smpp reportFlag[%d],source[%s],dest[%s].",_registeredDelivery,_sourceAddr.data(),_destinationAddr.data());
        
        LogDebug("msg:[%s],esmClass[%d]",msg,_esmClass);
        if (reader && (_esmClass&0x3c) == 4)
        {
            std::string stat;
            stat.reserve(20);
            std::string::size_type beginPos = _shortMessage.find("id:");
            std::string::size_type endPos = _shortMessage.find("sub:");
            std::string szMsgId;
            szMsgId.reserve(20);
            if (beginPos != std::string::npos && endPos != std::string::npos)
            {
                szMsgId=_shortMessage.substr(beginPos+3,endPos-1-beginPos-3);
            }

            beginPos = _shortMessage.find("stat:");
            endPos = _shortMessage.find("text:");
            if (beginPos != std::string::npos && endPos != std::string::npos)
            {
                _remark=_shortMessage.substr(beginPos,endPos-1-beginPos);
            }
            endPos = _shortMessage.find("err:");
            if (beginPos != std::string::npos && endPos != std::string::npos)
            {
                stat=_shortMessage.substr(beginPos+5,endPos-1-beginPos-5);
            }

            //2015-09-07 ADD by fangjinxiong
            if (stat.find("DELIVRD") != std::string::npos)
            {
                _status = 0;
            }

            _remark = stat;
            _msgId=szMsgId;
        }


        return reader;
    }

} /* namespace smsp */
