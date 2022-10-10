// Matt's charm os
// r13 set to 0x5000
// TODO - verify address of OS stack
.stack 0xcf00
.data 0xcf00
.label os_stack
0
// Base address of interrupt vector table
.data 0xff00
.label rupt_tab
mva pc, do_ker // branch to kerel mode rupt handler
mva pc, do_tmr // branch to tmr rupt handler

// process table
// 8 procs, each 64 bytes
// a process is structured as the following:
//    uint pid
//    uint state
//      UNUSED -> 0
//      EMBRYO -> 1
//      RUNNABLE -> 2
//      RUNNING -> 3
//      SLEEPING -> 4
//    uint startaddr
//    uint size in bytes
//    char *ustack
//    char *kstack
//    struct context *context
//    struct trapframe *tf
//    struct proc *parent
//    void *chan
//    uint killed
//    struct pde_t *pgdir
//    char[16] name

.data 0xef00
.label ptable
// process 1 0xef00
1       // pid
3       // state
0       // start addr
0       // size in bytes
0       // *ustack
0       // *kstack
0       // *context
0       // *tf
0       // *parent
0       // *chan
0       // killed
0       // *pgdir
0x67757374  // "gusty" 0xef30
0x79000000  // 0xef34
0x00000000  // 0xef38
0x00000000  // 0xef3c
// process 2 0xef40
2       // pid
2       // state
0       // start addr
0       // size in bytes
0       // *ustack
0       // *kstack
0       // *context
0       // *tf
0       // *parent
0       // *chan
0       // killed
0       // *pgdir
0x6c617572  // "lauren" 0xef70
0x656e0000  // 0xef74
0x00000000  // 0xef78
0x00000000  // 0xef7c
// process 3 0xef80
3       // pid
0       // state
0       // start addr
0       // size in bytes
0       // *ustack
0       // *kstack
0       // *context
0       // *tf
0       // *parent
0       // *chan
0       // killed
0       // *pgdir
0x0000  // ""
0x0000
0x0000
0x0000
// process 4 0xefc0
// process 5 0xf000
// process 6 0xf040
// process 7 0xf080
// process 8 0xf0c0
.data 0xf100
.label ptable_end

// kstack's are 256 bytes each, starting at 0xc000
// from 0xc000 to 0xc800
.data 0xc000
.label kstack
0
// ustacks are 256 bytes each, starting at 0x6000
// from 0x6000 to 0x68000
.data 0x06000
.label ustack
0
//.label forkret 0
// struct proc*
// pointer to current process running
.data 0xdf00
.label curr_proc
ptable

.data 0xdf04
.label sched_context
0xdf50  // points to mem for sched_context
0       // r4
0       // r5
0       // r6
0       // r7
0       // r8
0       // r9
0       // r10
0       // r11
0       // r12
0       // r13
0       // r14
0       // r15

// base address of os api, scanf, printf
// This code executes in kernel mode. It executes ioi
// This code is entered via ker instr
.text 0x9000
.label os_api
cmp r0, #0x10
beq scanf
cmp r0, #0x11
beq printf
bal done
.label scanf
ioi 0x10  // scanf
bal done
.label printf
ioi 0x11  // printf
.label done
mov r1, r0
srg #0x3b
mov r15, r14
//
// Address of printf is 0xa000, when called
//  r0 has addr of fmt string
//  r1 has first % variable, if any
//  r2 has second % variable, if any
// Called in user mode
.text 0xa000
str r14, [r13, #-4]! // push lr on stack
mov r3, r2           // set regs expected by ker 0x11
mov r2, r1
mov r1, r0
ker 0x11             // 0x11 is placed into r0
                     // user to kernel rupt is generated
ldr r14, [r13], #4   // pop lr from stack
mov r15, r14         // return
//
// Address of scanf is 0xa050, when called
//  r0 has addr of fmt string
//  r1 has first % variable, if any
//  r2 has second % variable, if any
// Called in user mode
.text 0xa050
str r14, [r13, #-4]! // push lr on stack
mov r3, r2           // set regs expected by ker 0x11
mov r2, r1
mov r1, r0
ker 0x10             // 0x10 is placed into r0
                     // user to kernel rupt is generated
ldr r14, [r13], #4   // pop lr from stack
mov r15, r14         // return


// struct trapframe {
//   uint sp; // user mode sp
//   uint r0;
//   uint r1; 
//   uint r2;
//   uint r3;
//   uint r4;
//   uint r5;
//   uint r6;
//   uint r7;
//   uint r8;
//   uint r9;
//   uint r10;
//   uint r11;
//   uint r12;
//   uint r13;
//   uint r14;
//   uint trapno; // 0x40 or 0x80
//   uint cpsr;
//   uint spsr; // saved cpsr rupted code
//   uint pc; // ret addr of rupted code
// };

// os code begins at address 0x8000
.text 0x8000
.label os_init
// establish interrupt vector table
mva r0, rupt_tab
mva r1, do_ker
mov r2, 0x81f
shf r2, 20
orr r1, r1, r2
str r1, [r0, 0]
mva r1, do_tmr
mov r2, 0x81f
shf r2, 20
orr r1, r1, r2
str r1, [r0, 4]
mva sp, os_stack
mkd r2, sp        // initialize kr13
mkd r5, sp        // initialize ir13
mov r0, 0
mva r1, 0x0200
mva r2, 0xef30
blr allocproc
mov r0, 1
mva r1, 0x0400
mva r2, 0xef70
blr allocproc
blr schedule

.label forkret
mov r15, r14      // returns to trapret()

// Handler is using appropriate sp?
// do_ker does not have to save r0-r3 because is is a function call?
// do_ker processes an user mode to kernel mode interrrupt - ker instruction
// Charm ker instr will generate this interrupt
// cpsr is in kpsr
// return addr is in kr14
// user mode sp is in kr13
.label do_ker
mkd r10, r0        // mov r0 into kr10, save r0 so we can use it
mks r0,  r6        // mks r0, ir14 // ret addr
str r0,  [sp, -4]!
mks r0,  r4        // mks r0, ipsr // user cpsr
str r0,  [sp, -4]!
mks r0,  r4        // mks r0, ipsr // user cpsr
str r0,  [sp, -4]!
mov r0,  #0x40
str r0,  [sp, -4]!   // save 0x40 on stack
// TODO Future - disable interrupts
str r14, [sp, -4]!  // save regs r14 to r0 on stack
str r13, [sp, -4]!
str r12, [sp, -4]!
str r11, [sp, -4]!
str r10, [sp, -4]!
str r9,  [sp, -4]!
str r8,  [sp, -4]!
str r7,  [sp, -4]!
str r6,  [sp, -4]!
str r5,  [sp, -4]!
str r4,  [sp, -4]!
str r3,  [sp, -4]!
str r2,  [sp, -4]!
str r1,  [sp, -4]!
mks r0,  r10       // restore r0 from kr10
str r0,  [sp, -4]!
mks r0,  r5        // mks r0, ir13 // user sp
str r0,  [sp, -4]!
mov r0, sp         // argument to trap(struct trapframe *tf)
blr trap

// TODO: Something is wrong with the trap frame offset; it's trying to push r15 too early
// trapret is used in allocproc(). lr = trapret
.label trapret
// What is in r13 when we get to here?
// r13 points to top of trap frame
//mov r0, sp // save sp in case it is changed to sp_usr after the following LDMFD instruction */
//ldmfd r0, {r13}^ /* restore user mode sp */
ldr r0,  [sp], 4 // restore user mode sp from trapframe
//mov sp, r0       // restore sp
ldr r0,  [sp], 4
ldr r1,  [sp], 4
ldr r2,  [sp], 4
ldr r3,  [sp], 4
ldr r4,  [sp], 4
ldr r5,  [sp], 4
ldr r6,  [sp], 4
ldr r7,  [sp], 4
ldr r8,  [sp], 4
ldr r9,  [sp], 4
ldr r10, [sp], 4
ldr r11, [sp], 4
ldr r12, [sp], 4
//ldr r13, [sp], 4
ldr r14, [sp], 0 // temporary junk
ldr r14, [sp], 4
ldr lr,  [sp], 4 // pop kpsr from trapframe
// kpsr or ipsr?
mkd r1, lr       // mks kpsr, lr
ldr pc,  [sp, 4] // pop pc from trapframe
// change mode???


// cpsr is in ipsr
// return addr is in ir14
// user mode sp is in ir13
.label do_tmr
mkd r10, r0        // mov r0 into kr10, save r0 so we can use it
mks r0,  r6        // mks r0, ir14 // ret addr
str r0,  [sp, -4]!
mks r0,  r4        // mks r0, ipsr // user cpsr
str r0,  [sp, -4]!
mks r0,  r4        // mks r0, ipsr // user cpsr
str r0,  [sp, -4]!
mov r0,  #0x80
str r0,  [sp, -4]!
// TODO Future - disable interrupts
// TODO Future - on page fault, retry instruction
// save regs on trapframe
str r14, [sp, -4]!
str r13, [sp, -4]!
str r12, [sp, -4]!
str r11, [sp, -4]!
str r10, [sp, -4]!
str r9,  [sp, -4]!
str r8,  [sp, -4]!
str r7,  [sp, -4]!
str r6,  [sp, -4]!
str r5,  [sp, -4]!
str r4,  [sp, -4]!
str r3,  [sp, -4]!
str r2,  [sp, -4]!
str r1,  [sp, -4]!
mks r0,  r10       // restore r0 from kr10
str r0,  [sp, -4]!
mks r0,  r5        // mks r0, ir13 // user sp
str r0,  [sp, -4]!
mov r0, sp         // argument to trap(struct trapframe *tf)
blr trap

// I AM HERE - return from blr trap
// Somehow r1 indicates return to user vs returning to kernel
mov r0, sp
add r0, r0, #76
//LDMIA r0, {r1} // load r1 from the stack, r0 has the stack address
mov r2, r1
and r2, r2, #0xf
cmp r2, #0
beq backtouser
//msr cpsr, r1
add sp, sp, #4
//LDMFD sp, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}
add sp, sp, #56
//pop {r14}
add sp, sp, #16
//pop {pc}

.label backtouser
mov r0, sp // save sp in case it is changed to sp_usr after the following LDMFD instruction */
//LDMFD r0, {r13}^ // restore user mode sp
mov r1, r1  // three nops after LDMFD
mov r1, r1
mov r1, r1
mov sp, r0  // restore sp
add sp, sp, #4
//LDMIA sp, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}
add sp, sp, #72
//pop {lr}
//msr spsr, lr
//pop {lr}
mov pc, lr  // subs pc,lr,#0

// sched calls swtch from an interrupt.
// scheduler calls swtch from kernel mode.
// r0 has context ** - switching from this context
// r1 has context *  - switching to this context
// r14 has return address
// struct context {
//   uint r4; uint r5; uint r6; uint r7; uint r8;
//   uint r9; uint r10; uint r11; uint r12;
//   uint lr; uint pc;
// };

.label swtch
ldr sp, [r0]

str lr,  [sp, -4]! // save return address, lr has return address
str lr,  [sp, -4]! // save lr
str r12, [sp, -4]! // save r12 through r4
str r11, [sp, -4]!
str r10, [sp, -4]!
str r9,  [sp, -4]!
str r8,  [sp, -4]!
str r7,  [sp, -4]!
str r6,  [sp, -4]!
str r5,  [sp, -4]!
str r4,  [sp, -4]!

// switch stacks
//str sp, [r0]      // lookie here: puts address into curr_proc->context
mov sp, r1

// load new callee-save registers
ldr r4,  [sp], 4  // restore r4 through r12
ldr r5,  [sp], 4
ldr r6,  [sp], 4
ldr r7,  [sp], 4
ldr r8,  [sp], 4
ldr r9,  [sp], 4
ldr r10, [sp], 4
ldr r11, [sp], 4
ldr r12, [sp], 4
ldr lr,  [sp], 4  // restore lr
ldr pc,  [sp], 4  // restore pc
//ldr r0, [sp], 4  // restore pc
//mov r15, r0

// r0 has addres of trap frame
.label trap
str lr,  [sp, -4]!
ldr r1, [r0, 64]  // put trap type (0x40, 0x80) in r1
cmp r1, 0x40      // 0x4 is system call (ker instr)
bne tmr_rupt      // timer interrupts
// see code in trap.c
mov pc, lr        // return for now. Later - call os_api
.label tmr_rupt
// check error conditions - see trap.c
ldr r1, curr_proc
cmp r1, 0
beq no_curr_proc
ldr r2, [r1, 32]  // 32 is offset of curr_proc->state
cmp r2, 4         // 4 is number for running
bne ret_from_trap
blr yield
.label no_curr_proc
.label ret_from_trap
ldr lr,  [sp], 4
mov pc, lr

.label yield
str lr,  [sp, -4]!
ldr r0, curr_proc
mov r1, 4         // 4 is the number for running
str r1, [r0, 32]  // 32 is offset of curr_proc->state
blr sched
ldr lr,  [sp], 4
mov pc, lr


//  intena = curr_cpu->intena;
//  swtch(&curr_proc->context, 
//      curr_cpu->scheduler);
//  curr_cpu->intena = intena;
.label sched
str lr,  [sp, -4]!
// Is this correct? curr_proc->context used to be pointer
ldr r0, curr_proc // addr of curr_proc to r0
add r0, r0, 12    // put address of curr_proc->context in r0, Update 12 to be correct
ldr r1, sched_context  // mov address of scheduler context to r0
blr swtch         // switch to scheduler's context
ldr lr,  [sp], 4
mov pc, lr

#define PROC_SIZE 64
#define KSTACK_SIZE 256
#define USTACK_SIZE 256
#define TF_SIZE 80
#define TF_USP 0
#define CONTEXT_SIZE 44
#define PROC_STATE 4
#define PROC_STARTADDR 8
#define PROC_USTACK 16
#define PROC_KSTACK 20
#define PROC_CONTEXT 24
#define PROC_NAME 48
#define PROC_TF 28
#define CONTEXT_PC 40
#define CONTEXT_LR 36
#define TF_PC 76

.label schedule
// sub sp, sp, []
// str lr, [sp, []]
// don't forget the first sched
.label for_loop_outer
//mva r0, schedcontext        // temporary; scheduler stack is current at 0x6000
//ldr sp, [r0, 36]
sub sp, sp, 4                 // allocate stack space for struct proc* p
mva r0, ptable
str r0, [sp, 0]               // p = &ptable
.label for_loop_inner
ldr r0, [sp, 0]
mva r1, ptable_end
cmp r0, r1                    // check if at end of ptable
bge for_loop_outer            // if so, move back to start of ptable
ldr r1, [r0, PROC_STATE]
cmp r1, 2                     // check if process is RUNNABLE
bne inner_incr
ldr r0, curr_proc             // change old curr_proc state to RUNNABLE
mov r1, 2
str r1, [r0, PROC_STATE]
ldr r0, [sp, 0]               // put p into r0
str r0, curr_proc
// switchuvm - later
mov r1, 3                     // change new curr_proc state to RUNNING
str r1, [r0, 0]
// call swtch
mva r0, 0xdf04                // &sched_context
ldr r1, curr_proc
ldr r1, [r1, PROC_CONTEXT]              // curr_proc->context
blr swtch
.label inner_incr
ldr r0, [sp, 0]
add r0, r0, PROC_SIZE
str r0, [sp, 0]
bal for_loop_inner
.label end
bal end

.label strcpy
mov r3, r0         // save dest str address
.label strcpyloop
ldb r2, [r1], 1    // char from src str
cmp r2, 0          // see if done
beq strcpydone     // yes
stb r2, [r0], 1    // place src str char into dest str
bal strcpyloop     // keep copying
.label strcpydone
mov r0, r3         // return dest str address
mov r15, r14       // return


// ptable, kstack, and ustack are sequential blocks
// r0 has index to allocate in ptable, kstack, ustack
// r1 has start address of proc (already loaded)
// r2 has address of proc's name (string)
// TODO: initialize pid, state, sz, parent, chan, killed, pgtbl during allocproc
.label allocproc
str r14, [sp, -4]!          // save lr on stack
str r2, [sp, -4]!           // save proc's name addr on stack
str r1, [sp, -4]!           // save proc's start addr on stack
mul r1, r0, PROC_SIZE       // mul by sizeof ptable entry
mva r2, ptable
add r2, r2, r1              // r2 has address of ptable entry to use
mul r1, r0, KSTACK_SIZE     // mul by sizeof kstack frame
mva r3, kstack
add r3, r3, r1              // r3 has address of kstack to use
add r3, r3, KSTACK_SIZE     // stacks grow backwards, r3 has addr of bottom of kstack
str r3, [r2, PROC_KSTACK]   // str to p->kstack
sub r3, r3, TF_SIZE         // sub sizeof trapframe, r3 has addr of trapframe
str r3, [r2, PROC_TF]       // str to p->tf
ldr r1, [sp, 0]             // retrieve start addr from stack
str r1, [r3, TF_PC]         // store start addr in tf->pc
sub r3, r3, CONTEXT_SIZE    // sub sizeof context, r3 has addr of context
str r3, [r2, PROC_CONTEXT]  // str to p->context
mul r1, r0, USTACK_SIZE     // mul by sizeof ustack
mva r3, ustack
add r3, r3, r1              // r3 has address of ustack to use
add r0, r3, USTACK_SIZE     // stacks grow backwards, r0 has addr of bottom of ustack
str r0, [r2, PROC_USTACK]   // str to p->ustack
ldr r0, [sp], 4             // retrieve proc's start addr from stack
str r0, [r2, PROC_STARTADDR]   // str to p->startaddr
//ldr r1, [sp], 4             // retrieve proc's name from stack
//add r0, r2, PROC_NAME       // address p->name to r0
//blr strcpy
mva r1, forkret
ldr r3, [r2, PROC_CONTEXT]  // retrieves context addr from proc
str r1, [r3, CONTEXT_PC]    // str to p->context->pc
mva r1, trapret
str r1, [r3, CONTEXT_LR]    // str p->context->lr

ldr r1, [sp], 4             // retrieve proc's name from stack
add r0, r2, PROC_NAME       // address p->name to r0
blr strcpy

ldr lr, [sp], 4             // retrieve lr from stack
mov pc, lr
