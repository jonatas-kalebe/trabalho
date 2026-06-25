#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"

void generate_code(Program *prog, FILE *out);

#endif
