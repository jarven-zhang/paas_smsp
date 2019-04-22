#include "TemplateGw.h"
namespace models
{

    TemplateGw::TemplateGw()
    {
        m_strChannelID= "";
        distoperators=0;
        policy=0;
        state=0;

        m_strYDChannelID = "";
        ydpolicy = 0;
        m_strLTChannelID ="";
        ltpolicy = 0;
        m_strDXChannelID ="";
        dxpolicy =0;
        m_strGJChannelID ="";
        gjpolicy =0;


    }

    TemplateGw::~TemplateGw()
    {

    }

    TemplateGw::TemplateGw(const TemplateGw& other)
    {

        this->m_strChannelID=other.m_strChannelID;
        this->templateid=other.templateid;
        this->distoperators=other.distoperators;
        this->policy=other.policy;
        this->state=other.state;
        this->remarks=other.remarks;

        this->m_strYDChannelID = other.m_strYDChannelID;
        this->ydpolicy = other.ydpolicy;
        this->m_strLTChannelID =other.m_strLTChannelID;
        this->ltpolicy = other.ltpolicy;
        this->m_strDXChannelID = other.m_strDXChannelID;
        this->dxpolicy = other.dxpolicy;
        this->m_strGJChannelID = other.m_strGJChannelID;
        this->gjpolicy = other.gjpolicy;
    }

    TemplateGw& TemplateGw::operator =(const TemplateGw& other)
    {
        this->m_strChannelID=other.m_strChannelID;
        this->templateid=other.templateid;
        this->distoperators=other.distoperators;
        this->policy=other.policy;
        this->state=other.state;
        this->remarks=other.remarks;

        this->m_strYDChannelID = other.m_strYDChannelID;
        this->ydpolicy = other.ydpolicy;
        this->m_strLTChannelID =other.m_strLTChannelID;
        this->ltpolicy = other.ltpolicy;
        this->m_strDXChannelID = other.m_strDXChannelID;
        this->dxpolicy = other.dxpolicy;
        this->m_strGJChannelID = other.m_strGJChannelID;
        this->gjpolicy = other.gjpolicy;

        return *this;
    }
} /* namespace models */

