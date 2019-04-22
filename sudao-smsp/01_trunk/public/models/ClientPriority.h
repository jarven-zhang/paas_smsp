#ifndef __CLIENTPRIORITY_H__
#define __CLIENTPRIORITY_H__

#include <string>
#include <map>

// MQ's priority
const int DEFAULT_CLIENT_PRIORITY = 0;
const int MAX_CLIENT_PRIORITY = 6;

class ClientPriority {
public:
    std::string m_strClientId;
    std::string m_strSign;
    UInt32 m_uSmsType;
    int m_iPriority;
};

typedef std::map<std::string, ClientPriority> client_priorities_t;

#endif
