#include "Cond.h"

boost::mutex g_mutexInitCv;
boost::condition_variable g_cv;
bool g_bMainInitOver = false;

