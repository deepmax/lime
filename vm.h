#ifndef VM_H
#define VM_H

#include "types.h"

#define EMIT(...) do{uint8_t b[] = { __VA_ARGS__ }; vm_code_emit(b, sizeof(b) / sizeof(b[0]));}while(0)
#define CODE(i, ...) do{uint8_t b[] = { __VA_ARGS__ }; vm_code_set(i, b, sizeof(b) / sizeof(b[0]));}while(0)
// #define DATA(...) do{uint8_t b[] = { __VA_ARGS__ }; vm_data_emit(b, sizeof(b) / sizeof(b[0]));}while(0)

enum
{
    NOP,
    DUP,
    DROP,
    ALLC,
    SWAP,
    PROC,
    CALL,
    RET,
    JNZ,
    JEZ,
    JMP,
    HALT,
    IINC,
    IDEC,
    INEG,
    IABS,
    INOT,
    IADD,
    ISUB,
    IDIV,
    IMOD,
    IMUL,
    IAND,
    IOR,
    IBXOR,
    IBOR,
    IBAND,
    ISHL,
    ISHR,
    IGT,
    ILT,
    IGE,
    ILE,
    IEQ,
    INQ,
    I8CONST,
    I16CONST,
    I32CONST,
    I64CONST,
    ICONST_0,
    ICONST_1,
    IPRINT,
    I8CAST,
    I16CAST,
    I32CAST,
    I64CAST,
    IU8CAST,
    IU16CAST,
    IU32CAST,
    IU64CAST,
    ILOAD,
    ISTORE,
    ITOR,

    // real
    RINC,
    RDEC,
    RNEG,
    RABS,
    RADD,
    RSUB,
    RDIV,
    RMOD,
    RMUL,
    RPOW,
    RSQRT,
    REXP,
    RSIN,
    RCOS,
    RTAN,
    RASIN,
    RACOS,
    RATAN2,
    RLOG,
    RLOG10,
    RLOG2,
    RCEIL,
    RFLOOR,
    RROUND,
    RGT,
    RLT,
    RGE,
    RLE,
    REQ,
    RNQ,
    RLOAD,
    RSTORE,
    RCONST,
    RCONST_0,
    RCONST_1,
    RCONST_PI,
    RPRINT,
    RTOI,
    XLOAD,
    XSTORE,
    XCONST,
    SPRINT,
    SLEN,
    AINIT,
    AINDXR,
    AINDXW,
    ALEN,
    NPRINT,
};

#define NUM64(X) \
    (X & 0xFF), \
    ((X >> 8) & 0xFF), \
    ((X >> 16) & 0xFF), \
    ((X >> 24) & 0xFF), \
    ((X >> 32) & 0xFF), \
    ((X >> 40) & 0xFF), \
    ((X >> 48) & 0xFF), \
    ((X >> 56) & 0xFF)

#define NUM32(X) \
    (X & 0xFF), \
    ((X >> 8) & 0xFF), \
    ((X >> 16) & 0xFF), \
    ((X >> 24) & 0xFF)

#define NUM16(X) \
    (X & 0xFF), \
    ((X >> 8) & 0xFF)

#define NUM8(X) \
    (X & 0xFF)

typedef struct
{
    const uint8_t code;
    const uint8_t arg_size;
    const char name[32];
} opcode_t;

extern const opcode_t OPCODES[];

void vm_init();
void vm_free();
void vm_exec();
void vm_dump();
void vm_dasm(const char* filename);
void vm_save(char* name);
void vm_load(char* name);
void vm_code_emit(uint8_t* bytes, size_t len);
void vm_code_set(size_t index, uint8_t* bytes, size_t len);
size_t vm_code_addr();
void vm_data_emit(uint8_t* bytes, size_t len);
uint8_t* vm_data_ptr();
size_t vm_data_used();

#endif /* VM_H */
