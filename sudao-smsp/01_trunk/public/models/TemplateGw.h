#ifndef TEMPLATE_GW_H_
#define TEMPLATE_GW_H_
#include "platform.h"

#include <string>
#include <iostream>
using namespace std;

namespace models
{

    class TemplateGw
    {
    public:
        TemplateGw();
        virtual ~TemplateGw();

        TemplateGw(const TemplateGw& other);

        TemplateGw& operator =(const TemplateGw& other);

    public:
        string m_strChannelID;
        string templateid;
        UInt32 distoperators;
        UInt32 policy;
        UInt32 state;
        string remarks;
        string m_strYDChannelID;
        UInt32 ydpolicy;
        string m_strLTChannelID;
        UInt32 ltpolicy;
        string m_strDXChannelID;
        UInt32 dxpolicy;
        string m_strGJChannelID;
        UInt32 gjpolicy;
    };

}

#endif

