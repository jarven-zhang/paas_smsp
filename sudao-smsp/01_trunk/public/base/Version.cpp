#include "./Version.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <time.h>

int GetColSize()
{
    struct winsize size;
    ioctl(STDIN_FILENO,TIOCGWINSZ,&size);
    return size.ws_col;
}

const char *  GetVersionInfo() 
{
    char head[256] = {0};
    static char buff[2048] = {0};
    char mid1[256] = {0};
    char mid2[256] = {0};
    int i = 0;
	char SubVersion[] = "6.5.0.0";
	char Version[] = "6.5.0";
	
	char c_time[128] = {0};
	time_t n_time;
	n_time = time(NULL);
	struct tm tm1;
	localtime_r(&n_time,&tm1 );
	sprintf(c_time,"%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d",
		tm1.tm_year+1900,tm1.tm_mon+1, tm1.tm_mday,
		tm1.tm_hour, tm1.tm_min,tm1.tm_sec);
	char szVer1[512] = {0};
	char szVer2[512] = {0};
	sprintf(szVer1, "\033[40;36msmsp_version_v%s Linux Build in %s\033[0m", SubVersion, c_time);
	sprintf(szVer2, "\033[40;36mSmsp Version V%s Copyright (c) 2014-2020 UcPaas.com\033[0m", Version);
	
	
    int lenVer1 = strlen(szVer1);
    int lenVer2 = strlen(szVer2);
    int size = GetColSize();
    int len1 = (size/2 - lenVer1)/2;
    int len2 = (size/2 - lenVer2)/2;
    strcpy(head, "\033[40;32m");
    for(i=0; i<size/2; i++)
        strcat (head,"=");
    if(size < lenVer1 || size < lenVer2 || 0 > len1 || 0 > len2)
    {
        for(i=0; i<size/2; i++)
            strcat (head,"=");
        len1 = (size - lenVer1)/2;
        len2 = (size - lenVer2)/2;
    }
    strcat(head, "\033[0m");
    for(i=0; i<len1; i++)
        strcat (mid1," ");
    for(i=0; i<len2; i++)
        strcat (mid2," ");
    sprintf(buff, "%s\n%s%s\n%s%s\n%s\n", head, mid1, szVer1, mid2, szVer2, head);
    return buff;
}

