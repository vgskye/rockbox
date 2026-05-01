#include <stdint.h>
#include <stdlib.h>
#include "autoconf.h"

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

uint32_t esp_random(void) {
	return rand();
}