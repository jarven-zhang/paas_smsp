#include "SGIPDeliverReq.h"
#include <stdio.h>
#include "UTFString.h"
#include "main.h"


SGIPDeliverReq::SGIPDeliverReq()
{
    m_strMsgContent = "";
    m_strPhone = "";
    m_strReserve = "";
    m_strSpNum = "";
    m_uMsgCoding = 8;
    m_uMsgLength = 0;
    m_uTpPid = 0;
    m_uTpUdhi = 0;
    requestId_ = SGIP_DELIVER;
}

SGIPDeliverReq::~SGIPDeliverReq()
{

}

bool SGIPDeliverReq::Unpack(Reader &reader)
{
    /*
    reader(21,m_strPhone)(21, m_strSpNum)(m_uTpPid)(m_uTpUdhi)(m_uMsgCoding)(m_uMsgLength);
    reader((UInt32) m_uMsgLength, m_strMsgContent)(8,m_strReserve);
    return reader;
    */

    return false;
}

UInt32 SGIPDeliverReq::bodySize() const
{
    return  m_uMsgLength + 21+21+1+1+1+4+8 + SGIPBase::bodySize();
}
bool SGIPDeliverReq::Pack(Writer &writer) const
{
    SGIPBase::Pack(writer);

    char cPhone[22] = {0};
    snprintf(cPhone,22,"%s",m_strPhone.data());

    char cSp[22] = {0};
    snprintf(cSp,22,"%s",m_strSpNum.data());

    char cReserve[9] = {0};

    writer((UInt32)21,cPhone)((UInt32)21,cSp)(m_uTpPid)(m_uTpUdhi)(m_uMsgCoding)
        (m_uMsgLength)((UInt32)m_uMsgLength,m_strMsgContent.data())((UInt32)8,cReserve);

    return writer;
}