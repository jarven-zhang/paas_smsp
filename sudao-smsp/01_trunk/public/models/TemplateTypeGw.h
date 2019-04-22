#ifndef TEMPLATETYPE_GW_H_
#define TEMPLATETYPE_GW_H_
#include "platform.h"

#include <string>
#include <iostream>
using namespace std;

namespace models
{

    class TemplateTypeGw
    {
    public:
        TemplateTypeGw();
        virtual ~TemplateTypeGw();

        TemplateTypeGw(const TemplateTypeGw& other);

        TemplateTypeGw& operator =(const TemplateTypeGw& other);

    public:
        UInt32 templatetype;
        string m_strChannelID;
        UInt32 distoperators;
        UInt32 policy;
        UInt32 state;
        //UInt32 autoswitch;
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

