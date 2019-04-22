#ifndef __AccessToken_H__
#define __AccessToken_H__
#include "platform.h"
#include <string>

namespace models
{
    class AccessToken
    {
    public:
        AccessToken();
        virtual ~AccessToken();
        AccessToken(const AccessToken& other);
        AccessToken& operator=(const AccessToken& other);

    public:
        std::string m_strAccessToken;
        Int64 m_iExpiresIn;
        std::string m_strRefreshToken;
        std::string m_strState;    //±£¡ÙŒª
		
    };
} /* namespace models */
#endif /* AccessToken_H_ */
