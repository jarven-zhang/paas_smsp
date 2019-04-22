#include "zlib/zlib.h"

#include "compress.h"

int CompressHelper::compress(COMPRESS_TYPE iCompressType, const std::string& rawData, std::string& compressedData)
{
    switch(iCompressType)
    {
        case GZIP:
            return gzipCompress(rawData, compressedData);
        default:
            return -888;
    }
}

int CompressHelper::decompress(COMPRESS_TYPE iCompressType, const std::string& compressedData, std::string& rawData)
{
    switch(iCompressType)
    {
        case GZIP:
            return gzipDecompress(compressedData, rawData);
        default:
            return -888;
    }
}

/* Compress gzip data */
/* data 原数据 ndata 原数据长度 zdata 压缩后数据 nzdata 压缩后长度 */
int CompressHelper::gzipCompress(const std::string& rawData, std::string& compressedData)
{
    if (rawData.empty())
    {
        return 0;
    }

    Bytef *data = (Bytef*)rawData.c_str();
    uLong ndata = (uLong)rawData.size(); 

    uLong nzdata = ndata*10;
    compressedData.resize(nzdata);
    Bytef *zdata = (Bytef*)compressedData.c_str();

    z_stream c_stream;
    int err = 0;

    if(data && ndata > 0) 
    {
        c_stream.zalloc = NULL;
        c_stream.zfree = NULL;
        c_stream.opaque = NULL;

        //只有设置为MAX_WBITS + 16才能在在压缩文本中带header和trailer
        if(deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                    MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) 
        {
            return -1;
        }

        c_stream.next_in  = data;
        c_stream.avail_in  = ndata;
        c_stream.next_out = zdata;
        c_stream.avail_out  = nzdata;
        while(c_stream.avail_in != 0 && c_stream.total_out < nzdata) 
        {
            if(deflate(&c_stream, Z_NO_FLUSH) != Z_OK) 
            {
                return -2;
            }
        }

        if(c_stream.avail_in != 0)
        {
            return c_stream.avail_in;
        }

        for(;;) 
        {
            if((err = deflate(&c_stream, Z_FINISH)) == Z_STREAM_END) 
                break;
            if(err != Z_OK)
            {
                return -3;
            }
        }

        if(deflateEnd(&c_stream) != Z_OK) 
        {
            return -4;
        }

        compressedData.resize(c_stream.total_out);

        return 0;
    }

    return -6;
}

/* Uncompress gzip data */
int CompressHelper::gzipDecompress(const std::string& compressedData, std::string& rawData)
{
    if (compressedData.empty())
    {
        return 0;
    }
    /* zdata 数据 nzdata 原数据长度 data 解压后数据 ndata 解压后长度 */
    Byte *zdata = (Byte*)compressedData.c_str();
    uLong nzdata = (uLong)compressedData.size();

    uLong ndata = nzdata * 8;
    rawData.resize(ndata);
    Byte *data = (Byte*)rawData.c_str();

    int err = 0;
    z_stream d_stream = {0}; /* decompression stream */
    static char dummy_head[2] = {
        0x8 + 0x7 * 0x10,
        (((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
    };

    d_stream.zalloc = NULL;
    d_stream.zfree = NULL;
    d_stream.opaque = NULL;
    d_stream.next_in = zdata;
    d_stream.avail_in = 0;
    d_stream.next_out = data;

    //只有设置为MAX_WBITS + 16才能在解压带header和trailer的文本
    if(inflateInit2(&d_stream, MAX_WBITS + 16) != Z_OK) 
    {
        return -1;
    }

    while(d_stream.total_out < ndata && d_stream.total_in < nzdata) 
    {
        d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
        if((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END) 
        {
            break;
        }
        if(err != Z_OK) 
        {
            if(err == Z_DATA_ERROR) 
            {
                d_stream.next_in = (Bytef*) dummy_head;
                d_stream.avail_in = sizeof(dummy_head);
                if((err = inflate(&d_stream, Z_NO_FLUSH)) != Z_OK) 
                {
                    return -1;
                }
            } 
            else 
            {
                return -1;
            }
        }
    }

    rawData.resize(d_stream.total_out);

    if(inflateEnd(&d_stream) != Z_OK) 
    {
        return -1;
    }

    return 0;
}
