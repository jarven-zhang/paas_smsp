#ifndef __LISTENPORT__
#define __LISTENPORT__

#include "platform.h"

#include <string>
#include <map>
#include <iostream>
using namespace std;

class ListenPort
{
public:
	ListenPort();
	~ListenPort();
	ListenPort(const ListenPort& other);
	ListenPort& operator=(const ListenPort& other);
public:
    string  m_strPortKey;
    UInt32  m_uPortValue;
    string  m_strComponentType;
};

typedef map<string, ListenPort> ListenPortMap;
typedef map<string, ListenPort>::iterator ListenPortMapIter;

#endif ////__LISTENPORT__
