#ifndef CMPPBASE_H_
#define CMPPBASE_H_
#include "platform.h"
#include <iostream>
#include <string.h>

#include "readerEx.h"
#include "writerEx.h"
#include "CmdCode.h"
#include <stdio.h>
#include <vector>


using namespace pack;
using namespace std;


//CMPP错误码
typedef enum
{
    CMPP_SUCCESS                    = 0,    //正确
    CMPP_ERROR_MSG_FORMAT           = 1,    //消息结构错
    CMPP_ERROR_COMMAND              = 2,    //命令字错
    CMPP_ERROR_REPEAT_MSGID         = 3,    //消息序号重复
    CMPP_ERROR_MSG_LENGTH           = 4,    //消息长度错
    CMPP_ERROR_FEECODE              = 5,    //资费代码错
    CMPP_ERROR_OVER_MAX_MSGLEN      = 6,    //超过最大信息长
    CMPP_ERROR_SERVICEID            = 7,    //业务代码错
    CMPP_ERROR_FLOW_CONCTROL        = 8,    //流量控制错
    CMPP_ERROR_SYS                  = 9,    //其他错误
}CMPP_ERROR_CODE;
//cmpp login error code
typedef enum
{
    CMPP_LOGIN_RESULT_OK               = 0,  //正确
    CMPP_LOGIN_RESULT_MSG_FORMAT_ERROR = 1,  //消息结构错误
    CMPP_LOGIN_RESULT_INVALID_SRC      = 2,  //非法源地址
    CMPP_LOGIN_RESULT_AUTH_ERROR       = 3,  // 认证错
    CMPP_LOGIN_RESULT_VERSION          = 4,  //版本错误
    CMPP_LOGIN_RESULT_OTHER_ERROR      = 5,  //其他错误
}CMPP_LOGIN_ERROR_CODE;

namespace smsp
{
    char * timestamp(char *buf);
    char * timeYear(char *buf);
    class CMPPBase: public Packet
    {
    public:
        CMPPBase();
        virtual ~CMPPBase();
        CMPPBase(const CMPPBase& other)
        {
            _totalLength = other._totalLength;
            _commandId = other._commandId;
            _sequenceId = other._sequenceId;
        }

        CMPPBase& operator =(const CMPPBase& other)
        {
            _totalLength = other._totalLength;
            _commandId = other._commandId;
            _sequenceId = other._sequenceId;
            return *this;
        }
        const char* getCode(UInt32 code)const;
    public:

        virtual UInt32 bodySize() const
        {
            return 12;
        }
        virtual bool Pack(Writer &writer) const;
        virtual bool Unpack(Reader &reader);

        virtual std::string Dump() const
        {
            return "";
        }
        static UInt32 getSeqID();
        static UInt32 g_seqId;
    public:
        //header
        UInt32 _totalLength;
        UInt32 _commandId;
        UInt32 _sequenceId;
    private:

    };

} /* namespace smsp */
#endif /* CMPPBASE_H_ */
