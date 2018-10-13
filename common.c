#include "common.h"
#include <stdlib.h>
const char welcome_msg[] = "220 FTP Server ready.\r\n";
int getRandomInt(int min, int max)
{
    return rand() % (max - min) + min;
}