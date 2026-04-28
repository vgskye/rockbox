#include "osi/strncpy.h"
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#include "autoconf.h"

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;

    for(i = 0; i < n && *src; ++i)
        dest[i] = src[i];

    for(; i < n; ++i)
        dest[i] = 0;

    return dest;
}


int sprintf(char *buf, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(buf, INT_MAX, fmt, args);
    va_end(args);
    return ret;
}

uint32_t ntohl(uint32_t x)
{
#ifndef ROCKBOX_LITTLE_ENDIAN
	return x;
#else
	return ((((x) & 0xff000000) >> 24) | \
	        (((x) & 0x00ff0000) >>  8) | \
	        (((x) & 0x0000ff00) <<  8) | \
	        (((x) & 0x000000ff) << 24));
#endif
}

uint32_t htonl(uint32_t x)
{
#ifndef ROCKBOX_LITTLE_ENDIAN
	return x;
#else
	return ((((x) & 0xff000000) >> 24) | \
	        (((x) & 0x00ff0000) >>  8) | \
	        (((x) & 0x0000ff00) <<  8) | \
	        (((x) & 0x000000ff) << 24));
#endif
}

uint16_t ntohs(uint16_t x)
{
#ifndef ROCKBOX_LITTLE_ENDIAN
	return x;
#else
	return ((((x) & 0xff00) >> 8) | \
	        (((x) & 0x00ff) << 8));
#endif
}

uint16_t htons(uint16_t x)
{
#ifndef ROCKBOX_LITTLE_ENDIAN
	return x;
#else
	return ((((x) & 0xff00) >> 8) | \
	        (((x) & 0x00ff) << 8));
#endif
}