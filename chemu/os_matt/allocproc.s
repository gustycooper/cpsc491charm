.data 0x100
.label ptable
0
.label kstack
0
.label ustack
0
.label forkret
0
.label trapret
0

#define PROC_SIZE 32
#define KSTACK_SIZE 256
#define USTACK_SIZE 256
#define TF_SIZE 80
#define CONTEXT_SIZE 48
#define PROC_KSTACK 8
#define PROC_TF 24
#define PROC_SP 20
#define PROC_CONTEXT 28
#define CONTEXT_PC 44
#define CONTEXT_LR 36
#define GUSTY 100
#define COLETTA 8
#define psh(A) str A, [sp, -4]!
#define pop(A) ldr A, [sp], 4

.text 0x200
// ptable, kstack, and ustack are sequential blocks
// r0 has index to allocated in ptable, kstack, ustack
.label allocproc
mul r1, r0, PROC_SIZE       // mul by sizeof ptable entry
mva r2, ptable
add r2, r2, r1              // r2 has address of ptable entry to use
mul r1, r0, KSTACK_SIZE     // mul by sizeof kstack frame
mva r3, kstack
add r3, r3, r1              // r3 has address of kstack to use
add r3, r3, r1              // stacks grow backwards, r3 has addr of bottom of kstack
str r3, [sp, -4]!           // save kstack on stack
mul r1, r0, USTACK_SIZE     // mul by sizeof ustack
mva r3, ustack
add r3, r3, r1              // r3 has address of ustack to use
add r0, r3, r1              // stacks grow backwards, r3 has addr of bottom of ustack
sub r3, r3, TF_SIZE         // sub sizeof trapframe, r3 has addr of trapframe
str r3, [r2, PROC_TF]       // stro to p->tf
sub r3, r3, CONTEXT_SIZE    // sub sizeof context, r3 has addr of context
str r3, [r2, PROC_CONTEXT]  // str to p->context
str r0, [r2, PROC_SP]       // str to p->kstack - WRONG
ldr r0, [sp], 4             // retrieve addr of kstack from stack
str r0, [r2, PROC_KSTACK]   // str to p->kstack
mva r1, forkret
str r1, [r3, CONTEXT_PC]    // str to p->context->pc
mva r1, trapret
str r1, [r3, CONTEXT_LR]    // str p->context->lr
mov pc, lr
str r3, [sp, -4]!           // save kstack on stack
psh(r3)
ldr r0, [sp], 4             // retrieve addr of kstack from stack
pop(r3)
rfi 0
rfi 1
