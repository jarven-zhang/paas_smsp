#ifndef __MONITOR_DETAIL_OPTION_H_
#define __MONITOR_DETAIL_OPTION_H_

#include "platform.h"
#include <string>
#include <iostream>
using namespace std;

namespace models
{
    class MonitorDetailOption
    {
    public:
        MonitorDetailOption();
        virtual ~MonitorDetailOption();
        MonitorDetailOption(const MonitorDetailOption& other);
        MonitorDetailOption& operator =(const MonitorDetailOption& other);

    public:
		UInt32 m_uMeasurement;
		string m_strFieldName;
		Int32  m_uFieldType;
		Int32  m_uState; 
    };
};

#endif

