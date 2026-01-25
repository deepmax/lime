#include "parser.h"
#include "types.h"
#include "vm.h"
#include "jump.h"

int main(int argc, char *argv[])
{
    // vm_init();

    // JUMP_NEW(j);
    // EMIT(I8CONST, NUM8(0));
    // EMIT(DUP);
    // JUMP(JEZ, j);
    // EMIT(I8CONST, NUM8(1));
    // EMIT(IAND);
    // MARK(j);
    // // EMIT(I8CONST, NUM8(123));
    // EMIT(IPRINT, NUM8(MT_INT8));
    // EMIT(HALT);
    // JUMP_FIX(j);
    // JUMP_FREE(j);

    // JUMP_NEW(j);
    // JUMP(JMP, j);
    // EMIT(PROC, NUM16(2), NUM16(0));
    // EMIT(ILOAD, NUM16(0));
    // EMIT(ILOAD, NUM16(1));
    // EMIT(IADD);
    // EMIT(IPRINT, NUM8(MT_INT64));
    // EMIT(ICONST_0);
    // EMIT(RET);
    // MARK(j);
    // EMIT(I8CONST, NUM8(1));
    // EMIT(I8CONST, NUM8(2));
    // EMIT(CALL, NUM16(3));
    // EMIT(HALT);

    // JUMP_FIX(j);
    // JUMP_FREE(j);

    // vm_dasm("out.asm");
    // vm_exec();
    // vm_free();

    parser_load_file("examples/code0.lm");
    parser_parse();
    parser_free();

    return 0;
}
