#ifndef PHONEDAO_H_
#define PHONEDAO_H_

#include "platform.h"
#include "VerifySMS.h"
#include "BusTypes.h"

#include <string>
#include <vector>
#include <map>

using namespace std;


class PhoneDao
{
public:

    PhoneDao();
    virtual ~PhoneDao();
	void Init();
    UInt32 getPhoneType(cs_t phone);

	map<string, UInt32> m_OperaterSegmentMap;		//eg   134--1;
	VerifySMS*  m_pVerify;
};

#endif /* PHONEDAO_H_ */
