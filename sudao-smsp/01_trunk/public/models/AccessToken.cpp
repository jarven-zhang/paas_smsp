#include "AccessToken.h"

namespace models
{
    AccessToken::AccessToken()
    {

		m_strAccessToken = "";
        m_iExpiresIn = 0;
        m_strRefreshToken = "";
        m_strState = "";
    }

	AccessToken::~AccessToken()
	{
	}
	
    AccessToken::AccessToken(const AccessToken& other)
    {
        m_strAccessToken = other.m_strAccessToken;
        m_iExpiresIn = other.m_iExpiresIn;
        m_strRefreshToken = other.m_strRefreshToken;
        m_strState = other.m_strState;
    }

    AccessToken& AccessToken::operator =(const AccessToken& other)
    {
        m_strAccessToken = other.m_strAccessToken;
        m_iExpiresIn = other.m_iExpiresIn;
        m_strRefreshToken = other.m_strRefreshToken;
        m_strState = other.m_strState;
        return *this;
    }

} /* namespace smsp */

