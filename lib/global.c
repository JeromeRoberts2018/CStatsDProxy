#include "global.h"

void safeFree(void **pp) {
    if (*pp != NULL) {
        free(*pp);
        *pp = NULL;
    }
}
