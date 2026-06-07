#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common.c"
#include "str.c"
#include "lex.c"
#include "parse.c"
#include "irgen.c"
#include "sem.c"
#include "llvm.c"
#include "irid.c"
