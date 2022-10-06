void enable_intrs(void);  // enable interrupts
void disable_intrs(void); // disable interrupts
void tvinit(void); // initialize trap table
void trap_oops(struct trapframe *tf); // trap messed up
void handle_irq(struct trapframe *tf); // interrupts - tmr, disk, etc.

// system call, timer, disk, etc.
void trap(struct trapframe *tf) {
    intctrlregs *ip;
    uint istimer;

    if(tf->trapno == T_SYSCALL){
        if(curr_proc->killed)
            exit();
        curr_proc->tf = tf;  //  <<<<<-- lookie here
        syscall();
        if(curr_proc->killed)
            exit();
        return;
    }

    istimer = 0;
    switch (tf->trapno){
    case T_IRQ:
      // determine if timer or disk.
      // istimer = 1 if is a timer interrupts
      break;
    default: // some unexpected trapno
      if(curr_proc == 0 || (tf->spsr & 0xF) != USER_MODE){ // In kernel, it must be our mistake.
          cprintf("unexpected trap %d from cpu %d addr %x spsr %x cpsr %x ifar %x\n",
                  tf->trapno, curr_cpu->id, tf->pc, tf->spsr, tf->cpsr, tf->ifar);
          panic("trap");
      }
      // In user space, assume process misbehaved.
      cprintf("pid %d %s: trap %d on cpu %d "
              "addr 0x%x spsr 0x%x cpsr 0x%x ifar 0x%x--kill proc\n",
              curr_proc->pid, curr_proc->name, tf->trapno, curr_cpu->id, tf->pc,
              tf->spsr, tf->cpsr, tf->ifar);
      curr_proc->killed = 1;
    }

    if (curr_proc){
        if (curr_proc->killed && (tf->spsr&0xF) == USER_MODE)
            exit();

        if (curr_proc->state == RUNNING && istimer)
            yield();

        if (curr_proc->killed && (tf->spsr&0xF) == USER_MODE)
            exit();
    }
}
