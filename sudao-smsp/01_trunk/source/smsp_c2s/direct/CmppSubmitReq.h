#ifndef CMPPSUBMITREQH
#define CMPPSUBMITREQH
#include "CMPPBase.h"
namespace smsp
{

    class CmppSubmitReq: public CMPPBase
    {
    public:
        CmppSubmitReq();
        virtual ~CmppSubmitReq();
    public:
        virtual UInt32 bodySize() const;
        virtual bool Pack(Writer &writer) const;
        virtual bool Unpack(Reader &reader);
    public:
        void setContent(std::string content)
        {
            _msgContent=content;
            _msgLength = _msgContent.length();
        }

        void setContent(char* content,UInt32 len)
        {
            _msgContent.reserve(20480);
            _msgContent.append(content,len);
            _msgLength = len;
        }
        //CMPPSUBMIT 8,1,1,1,1,10,1,21,1,1,1,6,2,6,17,17,21,1,21,1,msglength,8
        std::string _msgSrc;
        std::string _phone; ///// ¦Ì??¡ão???,o????a¡Á?¡¤?¡ä?
        std::string _srcId;
        UInt8 _tpUdhi;
        UInt8 _msgFmt;
		std::string _serviceId;
		
    public:
        std::string _msgContent;
        UInt64 _msgId;
        UInt8 _pkTotal;
        UInt8 _pkNumber;
        UInt8 _regDelivery;
        UInt8 _msgLevel;
        UInt8 _feeUserType;
        UInt8 _tpPId;
		UInt8 _longSmProtocalHeadType;
		UInt8 _randCode8;   
		UInt16 _randCode;

        std::string _feeType;
        UInt8 _destUsrTl;   ///// ¦Ì??¡ão?????¨ºy
        UInt8 _msgLength;   //// ?¨²¨¨Y3¡è?¨¨
		UInt8 _phoneCount;  //// ¦Ì??¡ão?????¨ºy
		std::vector<std::string> _phonelist; /////¦Ì??¡ão????¡¥o?
    };

} /* namespace smsp */
#endif /* CMPPSUBMITREQH */
