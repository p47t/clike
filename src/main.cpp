#include <iostream>
#include "node.h"
#include "codegen.h"

using namespace clike;

extern NBlock* programBlock;
extern int yyparse();

int main(int argc, char **argv)
{
    yyparse();
    std::cout << programBlock << std::endl;
    
    CodeGenContext context;
    context.generateCode(*programBlock);
    context.runCode();

    return 0;
}
