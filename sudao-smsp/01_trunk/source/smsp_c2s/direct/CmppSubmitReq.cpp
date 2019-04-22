#include "CmppSubmitReq.h"
#include "UTFString.h"
#include <stdio.h>
#include <stdlib.h>
#include "main.h"

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
		_phoneCount = 0;


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
        char Fee_terminal_Id[21] = { 0 };
        snprintf(Fee_terminal_Id, 21,"%s", "00000000000");
        char feeCode[6] = { 0 };
        snprintf(feeCode, 6,"%s", "0000");
        char valIdTime[17] = { 0 };
        char atTime[17] = { 0 };
        char srcID[30] = { 0 };
        snprintf(srcID, 30,"%s", _srcId.data());
        const char reserve[8] = { '0' };
        char destTerminalId[21] = { 0 };

        snprintf(destTerminalId, 21,"%s", _phone.data());
        writer(_msgId)(_pkTotal)(_pkNumber)(_regDelivery)(_msgLevel)((UInt32) 10,
                _serviceId)(_feeUserType)((UInt32) 21, Fee_terminal_Id)(_tpPId)(
                    _tpUdhi)(_msgFmt)((UInt32) 6, _msgSrc.data())((UInt32) 2, _feeType)(
                        (UInt32) 6, feeCode)((UInt32) 17, valIdTime)((UInt32) 17, atTime)(
                            (UInt32) 21, srcID)(_destUsrTl)((UInt32) 21, destTerminalId)(
                                _msgLength)((UInt32) _msgLength, _msgContent.data())((UInt32) 8,
                                        reserve);

        return writer;
    }

    bool CmppSubmitReq::Unpack(Reader& reader)
    {
        if(reader.GetSize() == 0)
			return false;
		char Fee_terminal_Id[22] = { 0 };
		char feeCode[7] = { 0 };
		char valIdTime[18] = { 0 };
        char atTime[18] = { 0 };
		char reserve[9] = { '0' };
		char TmpMsgSrc[7] = { 0 };
		string onePhoneTmp = "";
		string onePhone;
		UInt8 tmp1, tmp2, tmp3, tmp4;
		
		reader(_msgId)(_pkTotal)(_pkNumber)(_regDelivery)(_msgLevel)((UInt32) 10,_serviceId)(_feeUserType)
            ((UInt32) 21, Fee_terminal_Id)(_tpPId)(_tpUdhi)(_msgFmt)((UInt32) 6, TmpMsgSrc)((UInt32) 2, _feeType)
            ((UInt32) 6, feeCode)((UInt32) 17, valIdTime)((UInt32) 17, atTime)((UInt32) 21, _srcId)(_destUsrTl);

		int i;
		for(i = 0; i < _destUsrTl; i++)
		{
			if(!reader((UInt32) 21, onePhoneTmp))
				return reader;

			onePhone = onePhoneTmp.data();			
			_phonelist.push_back(onePhone);
			///if(i == 0)
			////	_phone = onePhone;
			////else
			////	_phone += "," + onePhone;
		}		
		_phoneCount = i;

		reader(_msgLength);
		if(_pkTotal > 1 && _tpUdhi == 1)
		{
			reader(_longSmProtocalHeadType);
			if(_longSmProtocalHeadType == 5)
			{
				reader(tmp1)(tmp2)(_randCode8);
				_randCode = _randCode8;
				reader(tmp3)(tmp4);
				
				_msgLength = _msgLength - 6;
			}
			else if(_longSmProtocalHeadType == 6)
			{
				reader(tmp1)(tmp2)(_randCode);
				reader(tmp3)(tmp4);
				_msgLength = _msgLength - 7;
			}
			reader((UInt32) _msgLength, _msgContent);
		}
		else
		{
			reader((UInt32) _msgLength, _msgContent);
		}
		reader((UInt32) 8,reserve);
   
        return reader;
    }

} /* namespace smsp */
