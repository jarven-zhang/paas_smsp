#ifndef __SNMANAGER__
#define __SNMANAGER__

#include "platform.h"
/*
class SnManager
{
public:
    SnManager();
    virtual ~SnManager();
    UInt32 getSn()
    {
        if(_sn>0xFFFFFFFE)
        {
            _sn=0;
        }
        return _sn++;
    }
private:
    UInt32 _sn;
};
*/

class SnManager
{
public:
    SnManager();
    virtual ~SnManager();
    UInt64 getSn();
	UInt64 CreateSubmitId(UInt64 SwitchNumber);
    UInt32 getDirectSequenceId();
	UInt64 getSeq64(int which = 1);
	void   setSeq64(int which, UInt64 start);
	UInt64 CreateMoId(UInt64 SwitchNumber);
	void   Init(UInt32 uNodeId);
	
private:
    UInt32 _sn;
	UInt16 _sn16;
	UInt16 _sn16Mo;
	UInt64 _sn64;
	UInt64 _sn64Two;
    UInt32 m_uDirectSequenceId;
	UInt32 m_uNodeId;
};

#endif ///__SNMANAGER__
