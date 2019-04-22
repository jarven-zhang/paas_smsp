#include "SmsChlInterval.h"


SmsChlInterval::SmsChlInterval()
{
    _itlValLev1 = 30;   //val for lev2, num of msg
    //_itlValLev2 = 20; //val for lev2

    _itlTimeLev1 = 20;      //time for lev1, time of interval
    _itlTimeLev2 = 1;       //time for lev 2
    //_itlTimeLev3 = 1;     //time for lev 3
}

SmsChlInterval::SmsChlInterval(const SmsChlInterval& other)
{
    _itlValLev1 = other._itlValLev1;
    //_itlValLev2 = other._itlValLev2;
    _itlTimeLev1 = other._itlTimeLev1;
    _itlTimeLev2 = other._itlTimeLev2;
    //_itlTimeLev3 = other._itlTimeLev3;
}

SmsChlInterval& SmsChlInterval::operator =(const SmsChlInterval& other)
{
    _itlValLev1 = other._itlValLev1;
    //_itlValLev2 = other._itlValLev2;
    _itlTimeLev1 = other._itlTimeLev1;
    _itlTimeLev2 = other._itlTimeLev2;
    //_itlTimeLev3 = other._itlTimeLev3;
    return *this;
}


