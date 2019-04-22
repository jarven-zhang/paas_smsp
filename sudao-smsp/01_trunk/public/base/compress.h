#ifndef __COMPRESS_H__
#define __COMPRESS_H__
 
#include <string>
#include <cstdio>

enum COMPRESS_TYPE
{
    NONE = -1,
    GZIP = 0
};

class CompressHelper
{
public:
    static int compress(COMPRESS_TYPE iCompressType, const std::string& rawData, std::string& compressedData);
    static int decompress(COMPRESS_TYPE iCompressType, const std::string& compressedData, std::string& decompressedData);

    static int gzipCompress(const std::string& rawData, std::string& compressedData);
    static int gzipDecompress(const std::string& compressedData, std::string& decompressedData);
};



#endif
