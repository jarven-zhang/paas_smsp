#ifndef USER_GW_H_
#define USER_GW_H_
#include "platform.h"

#include <string>
#include <iostream>
using namespace std;

namespace models
{

    class UserGw
    {
    public:
        UserGw();
        virtual ~UserGw();

        UserGw(const UserGw& other);

        UserGw& operator =(const UserGw& other);

    public:
        
        string userid;			
        UInt32 distoperators;	
        UInt32 upstream;		
        UInt32 report;			
        UInt32 m_uSmstype;		
		string m_strChannelID;		
        string m_strYDChannelID;	
        string m_strLTChannelID;	
        string m_strDXChannelID;	
        string m_strGJChannelID;	
		string m_strXNYDChannelID;		
		string m_strXNLTChannelID;	
		string m_strXNDXChannelID;	
		string m_strCSign;		
		string m_strESign;		
		//string m_strExtNo;		
		//UInt32 m_uSignPortLen;	
		//UInt32 m_uIsNeedSign;
		UInt32 m_uUnblacklist;
		UInt32 m_uFreeKeyword;//0:check system keyword; 1:don't check system keyword. by liaojialin 20170706
		UInt32 m_uFreeChannelKeyword;//0:check channel keyword; 1:don't check channel keyword. by liaojialin 20170804

		string m_strFailedReSendChannelID;
    };

}

#endif

