#include <iostream>
#include <fstream>
#include "node.h"
#include "codegen.h"

using namespace clike;

extern NBlock* programBlock;
extern int yyparse();

int main(int argc, char **argv)
{
    yyparse();

    CodeGenContext context;
    context.generateCode(*programBlock);
    context.runCode();

    return 0;
}
