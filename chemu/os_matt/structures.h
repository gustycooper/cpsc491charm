// trap frame built on the stack by exception.s, and passed to trap().
struct trapframe {  // #define TF_SIZE 80
  uint sp;     // ( 0)user mode sp
  uint r0;     // ( 4)
  uint r1;     // ( 8)
  uint r2;     // (12)
  uint r3;     // (16)
  uint r4;     // (20)
  uint r5;     // (24)
  uint r6;     // (28)
  uint r7;     // (32)
  uint r8;     // (36)
  uint r9;     // (40)
  uint r10;    // (44)
  uint r11;    // (48)
  uint r12;    // (52)
  uint r13;    // (56)
  uint r14;    // (60)
  uint trapno; // (64)
  //uint ifar; // (  )Instruction Fault Address Register (IFAR)
  uint cpsr;   // (68)
  uint spsr;   // (72) saved cpsr from the trapped/interrupted mode
  uint pc;     // (76) return address of the interrupted code
};

struct cpu {                   // Per-CPU state
  uint id;                     // Local APIC ID; index into cpus[] below
  struct context *scheduler;   // swtch() here to enter scheduler
  uint started;                // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  struct cpu *cpu;
  struct proc *proc;           // The currently-running process.
};

#define NCPU 1
struct cpu cpus[NCPU];
#define curr_cpu (&cpus[0])
#define curr_proc   (cpus[0].proc)

// #define CONTEXT_SIZE 48
struct context { //   v--- byte offset in struct context
  uint r4;       // ( 0)
  uint r5;       // ( 4)
  uint r6;       // ( 8)
  uint r7;       // (12)
  uint r8;       // (16)
  uint r9;       // (20)
  uint r10;      // (24)
  uint r11;      // (28)
  uint r12;      // (32)
  uint lr;       // (36)
  uint pc;       // (44)
};

enum procstate { UNUSED=0, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// #define PROC_SIZE 64 : it is 52 bytes, but we allocate 0x40 (64)
struct proc {              //   v--- byte offset in struct proc
  int pid;                 // ( 0) Process ID
  enum procstate state;    // ( 4) Process state
  uint startaddr           // ( 8) Start address of code
  uint sz;                 // (12) Size of process memory (bytes)
  char *ustack;            // (16) Bottom of user stack for this process
  char *kstack;            // (20) Bottom of kernel stack for this process
  struct context *context; // (24) swtch() here to run process
  struct trapframe *tf;    // (28) Trap frame for current syscall
  struct proc *parent;     // (32) Parent process
  void *chan;              // (36) If non-zero, sleeping on chan
  int killed;              // (40) If non-zero, have been killed
  pde_t* pgdir;            // (44) Page table (currently not used)
  char name[16];           // (48) Process name (debugging)
};
