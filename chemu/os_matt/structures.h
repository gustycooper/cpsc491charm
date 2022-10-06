// Layout of the trap frame built on the stack
// by exception.s, and passed to trap().
// #define TF_SIZE 80
struct trapframe {
  uint sp; // user mode sp
  uint r0;
  uint r1; 
  uint r2;
  uint r3;
  uint r4;
  uint r5;
  uint r6;
  uint r7;
  uint r8;
  uint r9;
  uint r10;
  uint r11;
  uint r12;
  uint r13;
  uint r14;
  uint trapno;
  //uint ifar; // Instruction Fault Address Register (IFAR)
  uint cpsr;
  uint spsr; // saved cpsr from the trapped/interrupted mode
  uint pc; // return address of the interrupted code
};



// Per-CPU state
struct cpu {
  uchar id;                    // Local APIC ID; index into cpus[] below
  struct context *scheduler;   // swtch() here to enter scheduler
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  
  // Cpu-local storage variables; see below
  struct cpu *cpu;
  struct proc *proc;           // The currently-running process.
};

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

// Per-process state - 56 bytes - allocate 64, which is 0x40
// Eight entry ptable needs 512 bytes, which is hex 0x200
// #define PROC_SIZE 64
struct proc {              //   v--- byte offset in struct proc
  uint sz;                 // ( 0) Size of process memory (bytes)
  pde_t* pgdir;            // ( 4) Page table
  char *kstack;            // ( 8) Bottom of kernel stack for this process
  enum procstate state;    // (12) Process state
  volatile int pid;        // (16) Process ID
  struct proc *parent;     // (20) Parent process
  struct trapframe *tf;    // (24) Trap frame for current syscall
  struct context *context; // (28) swtch() here to run process
  void *chan;              // (32) If non-zero, sleeping on chan
  int killed;              // (36) If non-zero, have been killed
  char name[16];           // (40) Process name (debugging)
};
