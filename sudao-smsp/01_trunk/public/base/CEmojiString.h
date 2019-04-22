#ifndef BASE_CEMOJISTRING_H_
#define BASE_CEMOJISTRING_H_

#include <string.h>

#define EMOJI_FILLING_STRING ":emoji"

class CEmojiString
{
public:
	static bool fillEmojiString(const char * src, char * dst);
	static bool fillEmojiRealString(const char * src, char * dst);

private:
	static bool isUtf8(char strWord);
};
#endif