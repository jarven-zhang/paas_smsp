#include "CMPP3SubmitReq.h"
#include "UTFString.h"
#include<stdio.h>
#include <stdlib.h>
namespace cmpp3
{

    CMPPSubmitReq::CMPPSubmitReq()
    {
        _msgId = 0;
        _commandId=CMPP_SUBMIT;
        _phone.reserve(32);
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
		_feeTerminalType = 0;
		_destTerminalType = 0;
		_linkID = "";

    }

    CMPPSubmitReq::~CMPPSubmitReq()
    {

    }
	
    UInt32 CMPPSubmitReq::bodySize() const
    {
        return _msgContent.size() + 183 + CMPPBase::bodySize();
    }

    
    bool CMPPSubmitReq::Pack(Writer& writer) const
    {    	
        CMPPBase::Pack(writer);
        char serviceId[11] = {0};
        snprintf(serviceId,11,"%s",_serviceId.data());
        
        char Fee_terminal_Id[33] = {0};
		
        char feeCode[7] = {0};
        snprintf(feeCode, 7,"%s", "0000");

        char valIdTime[18] = {0};
        char atTime[18] = {0};

        char srcID[22] = {0};
        snprintf(srcID, 22,"%s", _srcId.data());
        
        char destTerminalId[33] = {0};
        snprintf(destTerminalId, 33,"%s", _phone.data());

        char msgSrc[7] = {0};
        snprintf(msgSrc,7,"%s",_msgSrc.data());

        char feeType[3] = {0};
        snprintf(feeType,3,"%s",_feeType.data());
		writer(_msgId)(_pkTotal)(_pkNumber)(_regDelivery)(_msgLevel)((UInt32) 10,serviceId)(_feeUserType)
			((UInt32) 32, Fee_terminal_Id)(_feeTerminalType)(_tpPId)(_tpUdhi)(_msgFmt)((UInt32) 6, msgSrc)
            ((UInt32) 2, feeType)((UInt32) 6, feeCode)((UInt32) 17, valIdTime)((UInt32) 17, atTime)
            ((UInt32) 21, srcID)(_destUsrTl)((UInt32) 32, destTerminalId)(_destTerminalType)(_msgLength)
            ((UInt32) _msgLength, _msgContent.data())((UInt32) 20,_linkID.data());

        return writer;
    }

    bool CMPPSubmitReq::Unpack(Reader& reader)
    {
        if(reader.GetSize() == 0)
			return false;
		char Fee_terminal_Id[22] = { 0 };
		char feeCode[7] = { 0 };
		char valIdTime[18] = { 0 };
        char atTime[18] = { 0 };
		char TmpMsgSrc[7] = { 0 };
		string onePhoneTmp = "";
		string onePhone;
		UInt8 tmp1, tmp2, tmp3, tmp4;
		
		reader(_msgId)(_pkTotal)(_pkNumber)(_regDelivery)(_msgLevel)((UInt32) 10,_serviceId)(_feeUserType)
            ((UInt32) 32, Fee_terminal_Id)(_feeTerminalType)(_tpPId)(_tpUdhi)(_msgFmt)((UInt32) 6, TmpMsgSrc)
            ((UInt32) 2, _feeType)((UInt32) 6, feeCode)((UInt32) 17, valIdTime)((UInt32) 17, atTime)
            ((UInt32) 21, _srcId)(_destUsrTl);

		int i;
		for(i = 0; i < _destUsrTl; i++)
		{
			if(!reader((UInt32) 32, onePhoneTmp))
				return reader;

			onePhone = onePhoneTmp.data();			
			_phonelist.push_back(onePhone);
		}		
		_phoneCount = i;

		reader(_destTerminalType)(_msgLength);
		if(_pkTotal > 1 && _tpUdhi == 1)
		{
			reader(_longSmProtocalHeadType);
			if(_longSmProtocalHeadType == 5)
			{
				reader(tmp1)(tmp2)(_randCode8);
				_randCode = _randCode8;//批短信的唯一标志
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
		reader((UInt32) 20,_linkID);
   
        return reader;
    }

} /* namespace smsp */
