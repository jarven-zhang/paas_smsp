#include "TemplateTypeGw.h"
namespace models
{

    TemplateTypeGw::TemplateTypeGw()
    {
        m_strChannelID= "";
        templatetype=0;
        policy=0;
        distoperators=0;
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

    TemplateTypeGw::~TemplateTypeGw()
    {

    }

    TemplateTypeGw::TemplateTypeGw(const TemplateTypeGw& other)
    {

        this->m_strChannelID=other.m_strChannelID;
        this->templatetype=other.templatetype;
        this->distoperators=other.distoperators;
        this->state=other.state;
        this->policy=other.policy;
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

    TemplateTypeGw& TemplateTypeGw::operator =(const TemplateTypeGw& other)
    {
        this->m_strChannelID=other.m_strChannelID;
        this->templatetype=other.templatetype;
        this->distoperators=other.distoperators;
        this->state=other.state;
        this->policy=other.policy;
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

