#include "framecounter.h"

unsigned counter = 0;

void frameCompleted()
{
    counter++;
}
unsigned frameCounter()
{
    return counter;
}