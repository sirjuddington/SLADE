
#ifndef __CODEPAGES_H__
#define __CODEPAGES_H__

#include "Main.h"

namespace CodePages
{
	string	fromASCII(uint8_t val);
	string	fromCP437(uint8_t val);
	rgba_t	ansiColor(uint8_t val);
};

#endif //__CODEPAGES_H__
