#include "CEmojiString.h"

bool CEmojiString::isUtf8(char    strWord)
{
	unsigned char ch = (unsigned char)strWord;
	if(ch >= 0x80 && ch <= 0xBF)
	{
		return true;
	}
	
	return false;
}


bool CEmojiString::fillEmojiString(const char * src, char * dst)
{
	int i = 0;
	int j = 0;

	if(src == NULL || dst == NULL){
		return false;
	}

	int len = strlen(src);
	
	while(i < len)
	{
		unsigned char cur = (unsigned char)src[i];

		if(((cur>>7)^0x00) == 0)
		{
			dst[j++] = src[i++];
		}
		else if(((cur>>5)^0x06) == 0)
		{
			if (isUtf8(src[i+1]))
			{
				strncpy(dst + j, src + i, 2);
				i += 2;
				j += 2;
			}
			else
			{
				strcat(dst,"?");
				i += 2;
				j += 1;
			}
			
		}
		else if(((cur >> 4) ^ 0x0E) == 0)
		{
			if (i + 3 <= len && isUtf8(src[i+1]) && isUtf8(src[i+2]))
			{
				strncpy(dst + j, src + i, 3);
				i += 3;
				j += 3;
			}
			else
			{
				strcat(dst,"?");
				i += 2;
				j += 1;
			}
		}
		else if(((cur >> 3) ^ 0x1E) == 0)
		{
			strcat(dst,"?");
			if (i + 4 <= len && isUtf8(src[i+1]) && isUtf8(src[i+2])&& isUtf8(src[i+3]))
			{
				i += 4;
				j += 1;
			}
			else
			{
				i += 2;
				j += 1;
			}
		}
		else if (((cur >> 2) ^ 0x3E) == 0)
		{
			strcat(dst,"?");
			if (i + 5 <= len && isUtf8(src[i+1]) && isUtf8(src[i+2])&& isUtf8(src[i+3]) && isUtf8(src[i+4]))
			{
				i += 5;
				j += 1;
			}
			else
			{
				i += 2;
				j += 1;
			}
		}
		else if(((cur >> 1) ^ 0x7E) == 0)
		{
			strcat(dst,"?");
			if (i + 6 <= len && isUtf8(src[i+1]) && isUtf8(src[i+2])&& isUtf8(src[i+3]) && isUtf8(src[i+4]) && isUtf8(src[i+4]))
			{
				i += 6;
				j += 1;
			}
			else
			{
				i += 2;
				j += 1;
			}
		}
		else
		{
			strcat(dst,"?");
			i += 2;
			j += 1;
		}

    }
	return true;
}

//\xF0\x9F\x80\x80 - \xF0\x9F\x89\x91  
//\xF0\x9F\x8C\x80 - \xF0\x9F\x99\x8f  
//\xF0\x9F\x9a\x80- \xF0\x9F\x9B\x85

bool CEmojiString::fillEmojiRealString(const char * src, char * dst)
{
	int i = 0;
	int j = 0;

	if(src == NULL || dst == NULL){
		return false;
	}

	int len = strlen(src);
	
	while(i < len)
	{
		unsigned char cur = (unsigned char)src[i];

		if(((cur>>7)^0x00) == 0)
		{
			dst[j++] = src[i++];
		}
		else if(((cur>>5)^0x06) == 0)
		{
			if (isUtf8(src[i+1]))
			{
				strncpy(dst + j, src + i, 2);
			}
			else
			{
				strcat(dst,"??");
			}
			i += 2;
			j += 2;
		}
		else if(((cur >> 4) ^ 0x0E) == 0)
		{
			if (i + 3 <= len && isUtf8(src[i+1]) && isUtf8(src[i+2]))
			{
				strncpy(dst + j, src + i, 3);
				i += 3;
				j += 3;
			}
			else
			{
				strcat(dst,"??");
				i += 2;
				j += 2;
			}
		}
		else if(((cur >> 3) ^ 0x1E) == 0)
		{
			if (i + 4 <= len && isUtf8(src[i+1]) && isUtf8(src[i+2])&& isUtf8(src[i+3]))
			{
				if (0xF0==(unsigned char)src[i] && 0x9F==(unsigned char)src[i+1])
				{
					if (((unsigned char)src[i+2]>=0x80 && (unsigned char)src[i+2]<=0x89 && (unsigned char)src[i+3]>=0x80 && (unsigned char)src[i+3]<=0x91)
						|| ((unsigned char)src[i+2]>=0x8C && (unsigned char)src[i+2]<=0x99 && (unsigned char)src[i+3]>=0x80 && (unsigned char)src[i+3]<=0x8f)
						|| ((unsigned char)src[i+2]>=0x9a && (unsigned char)src[i+2]<=0x9b && (unsigned char)src[i+3]>=0x80 && (unsigned char)src[i+3]<=0x85))
					{
						strcat(dst, EMOJI_FILLING_STRING);
						i += 4;
						j += 6;
					}
					else
					{
						strcat(dst,"????");
						i += 4;
						j += 4;
					}
				}
				else
				{
					strcat(dst,"????");
					i += 4;
					j += 4;
				}
			}
			else
			{
				strcat(dst,"??");
				i += 2;
				j += 2;
			}
		}
		else if (((cur >> 2) ^ 0x3E) == 0)
		{
			if (i + 5 <= len && isUtf8(src[i+1]) && isUtf8(src[i+2])&& isUtf8(src[i+3]) && isUtf8(src[i+4]))
			{
				strcat(dst,"?????");
				i += 5;
				j += 5;
		
			}
			else
			{
				strcat(dst,"??");
				i += 2;
				j += 2;
			}
		}
		else if(((cur >> 1) ^ 0x7E) == 0)
		{
			if (i + 6 <= len && isUtf8(src[i+1]) && isUtf8(src[i+2])&& isUtf8(src[i+3]) && isUtf8(src[i+4]) && isUtf8(src[i+4]))
			{
				strcat(dst,"??????");
				i += 6;
				j += 6;
			}
			else
			{
				strcat(dst,"??");
				i += 2;
				j += 2;
			}
		}
		else
		{
			strcat(dst,"??");
			i += 2;
			j += 2;
		}

    }
	return true;
}



