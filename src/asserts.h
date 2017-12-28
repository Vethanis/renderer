#pragma once

#define ASSERTS_ENABLED 1

#if ASSERTS_ENABLED

#include "stdio.h"
#include "stdlib.h"

#define Assert(x) { if(!(x)){ printf("Assertion failed: %s.\t%s :: %i\n", #x, __FILE__, __LINE__); exit(1); }}

#else

#define Assert(x) 

#endif // #if ASSERTS_ENABLED