#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "platform.h"

#include <string>
#include <vector>

using std::string;
using std::vector;

class CThread;

class ThreadPool
{
public:
    ThreadPool();
    virtual ~ThreadPool();

    virtual bool start() = 0;

    string getThreadPoolName() {return m_strThreadPoolName;}

    vector<CThread*> getAllThreads() {return m_vecThreads;}

private:
    ThreadPool(const ThreadPool&);
    ThreadPool& operator=(const ThreadPool&);

protected:
    // thread pool name
    string m_strThreadPoolName;

    // The number of threads in the thread pool
    UInt32 m_uiThreadNum;

    // The index of current thread in the thread pool
    UInt32 m_uiIndex;

    vector<CThread*> m_vecThreads;

private:

};

#endif // THREADPOOL_H
