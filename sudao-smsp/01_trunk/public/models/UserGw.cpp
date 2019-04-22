#include "UserGw.h"
namespace models
{

    UserGw::UserGw()
    {
        m_strChannelID= "";
        distoperators=0;
        upstream=0;
        report=0;
        m_strYDChannelID = "";
        m_strLTChannelID ="";
        m_strDXChannelID ="";
        m_strGJChannelID ="";
		m_uSmstype = 0;
		m_uUnblacklist = 0;
		m_uFreeKeyword = 0;
		m_uFreeChannelKeyword = 0;
		m_strFailedReSendChannelID = "";
    }

    UserGw::~UserGw()
    {

    }

    UserGw::UserGw(const UserGw& other)
    {
        this->m_strChannelID=other.m_strChannelID;
        this->userid=other.userid;
        this->distoperators=other.distoperators;
        this->upstream=other.upstream;
        this->report=other.report;
        this->m_strYDChannelID = other.m_strYDChannelID;
        this->m_strLTChannelID =other.m_strLTChannelID;
        this->m_strDXChannelID = other.m_strDXChannelID;
        this->m_strGJChannelID = other.m_strGJChannelID;
		this->m_strXNYDChannelID = other.m_strXNYDChannelID;
		this->m_strXNLTChannelID = other.m_strXNLTChannelID;
		this->m_strXNDXChannelID = other.m_strXNDXChannelID;
		this->m_uSmstype = other.m_uSmstype;
        this->m_strCSign = other.m_strCSign;
        this->m_strESign = other.m_strESign;
		this->m_uUnblacklist = other.m_uUnblacklist;
		this->m_uFreeKeyword = other.m_uFreeKeyword;
		this->m_uFreeChannelKeyword = other.m_uFreeChannelKeyword;
		this->m_strFailedReSendChannelID = other.m_strFailedReSendChannelID;
    }

    UserGw& UserGw::operator =(const UserGw& other)
    {
        this->m_strChannelID=other.m_strChannelID;
        this->userid=other.userid;
        this->distoperators=other.distoperators;
        this->upstream=other.upstream;
        this->report=other.report;
        this->m_strYDChannelID = other.m_strYDChannelID;
        this->m_strLTChannelID =other.m_strLTChannelID;
        this->m_strDXChannelID = other.m_strDXChannelID;
        this->m_strGJChannelID = other.m_strGJChannelID;
		this->m_strXNYDChannelID = other.m_strXNYDChannelID;
		this->m_strXNLTChannelID = other.m_strXNLTChannelID;
		this->m_strXNDXChannelID = other.m_strXNDXChannelID;
		this->m_uSmstype = other.m_uSmstype;
        this->m_strCSign = other.m_strCSign;
        this->m_strESign = other.m_strESign;
		this->m_uUnblacklist = other.m_uUnblacklist;
		this->m_uFreeKeyword = other.m_uFreeKeyword;
		this->m_uFreeChannelKeyword = other.m_uFreeChannelKeyword;
		this->m_strFailedReSendChannelID = other.m_strFailedReSendChannelID;
        return *this;
    }
} /* namespace models */

