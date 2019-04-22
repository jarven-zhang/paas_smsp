#include "cityHashShare.h"
#include "UTFString.h"
#include "CLogThread.h"

cityHashShard::cityHashShard() 
{
}

static bool is_str_utf8( const char* str)
{
  unsigned int nBytes = 0;//UFT8可用1-6个字节编码,ASCII用一个字节
  unsigned char chr = *str;
  bool bAllAscii = true;
  for (unsigned int i = 0; str[i] != '\0'; ++i){
    chr = *(str + i);
    //判断是否ASCII编码,如果不是,说明有可能是UTF8,ASCII用7位编码,最高位标记为0,0xxxxxxx
    if (nBytes == 0 && (chr & 0x80) != 0){
      bAllAscii = false;
    }

    if (nBytes == 0) {
      //如果不是ASCII码,应该是多字节符,计算字节数
      if (chr >= 0x80) {
        if (chr >= 0xFC && chr <= 0xFD){
          nBytes = 6;
        }
        else if (chr >= 0xF8){
          nBytes = 5;
        }
        else if (chr >= 0xF0){
          nBytes = 4;
        }
        else if (chr >= 0xE0){
          nBytes = 3;
        }
        else if (chr >= 0xC0){
          nBytes = 2;
        }
        else{
          return false;
        }
        nBytes--;
      }
    }
    else{
      //多字节符的非首字节,应为 10xxxxxx
      if ((chr & 0xC0) != 0x80){
        return false;
      }
      //减到为零为止
      nBytes--;
    }
  }
  //违返UTF8编码规则
  if (nBytes != 0) {
    return false;
  }
  if (bAllAscii){ //如果全部都是ASCII, 也是UTF8
    return true;
  }
  return true;
}

static bool is_str_gbk(const char* str)
{
  unsigned int nBytes = 0;//GBK可用1-2个字节编码,中文两个 ,英文一个
  unsigned char chr = *str;
  bool bAllAscii = true; //如果全部都是ASCII,
  for (unsigned int i = 0; str[i] != '\0'; ++i){
    chr = *(str + i);
    if ((chr & 0x80) != 0 && nBytes == 0){// 判断是否ASCII编码,如果不是,说明有可能是GBK
      bAllAscii = false;
    }
    if (nBytes == 0) {
      if (chr >= 0x80) {
        if (chr >= 0x81 && chr <= 0xFE){
          nBytes = +2;
        }
        else{
          return false;
        }
        nBytes--;
      }
    }
    else{
      if (chr < 0x40 || chr>0xFE){
        return false;
      }
      nBytes--;
    }//else end
  }
  if (nBytes != 0) {   //违返规则
    return false;
  }
  if (bAllAscii){ //如果全部都是ASCII, 也是GBK
    return true;
  }
  return true;
}

long cityHashShard::hash64( string s) 
{       
	utils::UTFString utfHelp;
	string out;
	if( is_str_utf8( s.c_str())){
		out = s ;
	}else if (is_str_gbk(s.c_str())){
		utfHelp.g2u( s, out );
	} else {
		utfHelp.Ascii2u( s, out );
	}
	LogDebug("len=[str_s:%d, str_d:%d, str_utf:%d ], str=[src:%s, dst:%s ]\n",
			s.length(), out.length(), utfHelp.getUtf8Length(out),
			s.c_str(), out.c_str() );

	return hash64( out.c_str(), out.length());
}

long cityHashShard::hash64(const char* data, int length) 
{
	return hash64(data, length, -294967317);
}

long cityHashShard::hash64(const char* data, int length, int seed) 
{
    long h1 = (long)seed & 4294967295L ^ (long)length;
    long h2 = 0L;

    int i;
    long h;
    for(i = 0; length - i >= 8; i += 4) {
        h = ((long)data[i + 0] & 255L) + (((long)data[i + 1] & 255L) << 8) + (((long)data[i + 2] & 255L) << 16) + (((long)data[i + 3] & 255L) << 24);
        h *= 1540483477L;
        h ^= h >> 24;
        h *= 1540483477L;
        h1 *= 1540483477L;
        h1 ^= h;
        i += 4;
        long k2 = ((long)data[i + 0] & 255L) + (((long)data[i + 1] & 255L) << 8) + (((long)data[i + 2] & 255L) << 16) + (((long)data[i + 3] & 255L) << 24);
        k2 *= 1540483477L;
        k2 ^= k2 >> 24;
        k2 *= 1540483477L;
        h2 *= 1540483477L;
        h2 ^= k2;
    }

    if (length - i >= 4) {
        h = ((long)data[i + 0] & 255L) + (((long)data[i + 1] & 255L) << 8) + (((long)data[i + 2] & 255L) << 16) + (((long)data[i + 3] & 255L) << 24);
        h *= 1540483477L;
        h ^= h >> 24;
        h *= 1540483477L;
        h1 *= 1540483477L;
        h1 ^= h;
        i += 4;
    }

    switch(length - i) {
    case 3:
        h2 ^= (long)(data[i + 2] << 16);
    case 2:
        h2 ^= (long)(data[i + 1] << 8);
    case 1:
        h2 ^= (long)data[i + 0];
        h2 *= 1540483477L;
    default:
        h1 ^= h2 >> 18;
        h1 *= 1540483477L;
        h2 ^= h1 >> 22;
        h2 *= 1540483477L;
        h1 ^= h2 >> 17;
        h1 *= 1540483477L;
        h2 ^= h1 >> 19;
        h2 *= 1540483477L;
        h = h1 << 32 | h2;
        return h;
    }
}
