#include <iostream>
#include "UTFString.h"
#include "platform.h"

class cityHashShard 
{
public :
	cityHashShard();
    static long hash64(std::string s); 
    static long hash64(const char* data, int length);
    static long hash64(const char* data, int length, int seed) ;
};
