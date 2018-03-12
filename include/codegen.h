#include <stack>
#include <map>

namespace clike {

class NBlock;

class CodeGenBlock {
public:
    llvm::BasicBlock* block;
    std::map <std::string, llvm::Value*> locals;
};

class CodeGenContext {
    // use a “stack” of blocks in our CodeGenContext class to keep the last entered block
    // (because instructions are added to blocks)
    std::stack <CodeGenBlock*> blocks;

    llvm::Function* mMainFunction;

public:
    llvm::Module* mModule;

    CodeGenContext();

    void generateCode(NBlock& root);
    void runCode();

    std::map <std::string, llvm::Value*>& locals() {
        return blocks.top()->locals;
    }

    llvm::BasicBlock* currentBlock() {
        return blocks.top()->block;
    }

    void pushBlock(llvm::BasicBlock* block) {
        blocks.push(new CodeGenBlock());
        blocks.top()->block = block;
    }

    void popBlock() {
        CodeGenBlock* top = blocks.top();
        blocks.pop();
        delete top;
    }
};

}
