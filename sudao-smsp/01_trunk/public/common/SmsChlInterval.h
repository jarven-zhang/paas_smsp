#ifndef SMSCHLINTERVAL_H_
#define SMSCHLINTERVAL_H_
#include <string>
#include <tr1/memory>
#include "platform.h"

class SmsChlInterval
{
public:
    SmsChlInterval();
    SmsChlInterval(const SmsChlInterval& other);
    SmsChlInterval& operator=(const SmsChlInterval& other);
    void release()
    {
        delete this;
    }

    /*
        num:        0--------------_itlValLev1---------------
        time:       [   _itlTimeLev1      ] [   _itlTimeLev2 ]
    */

    UInt32 _itlValLev1;    //val for lev2

    int _itlTimeLev1;       //time for lev1
    int _itlTimeLev2;       //time for lev 2

};

#endif /* SMSCHLINTERVAL_H_ */
