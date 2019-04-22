#ifndef THREAD_BASE_COND_H
#define THREAD_BASE_COND_H

#include "boost/thread/thread.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/thread/condition.hpp"


extern boost::mutex g_mutexInitCv;
extern boost::condition_variable g_cv;
extern bool g_bMainInitOver;


#endif // THREAD_BASE_COND_H
