#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <getopt.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include "memory.h"
#include "bit_functions.h"
#include "cpu.h"
#include "isa.h"
#include "dict.h"

/*
 Commands - see documentation for description of commands
 */

void printres(char *fmt, ...) __attribute__((format(printf, 1, 2)));

extern int verbose_cpu;
extern int total_steps;
extern int loop_stop;
/*
 Defines used to access members of OS struct proc.
 */
#define PROC_SIZE     64
#define PROC_PID      0
#define PROC_STATE    4
#define PROC_START    8
#define PROC_USTACK   16
#define PROC_KSTACK   20
#define PROC_CONTEXT  24
#define PROC_TRAP     28
#define PROC_NAME     48
#define TF_PC         76
#define STATE_READY   2
#define STATE_RUN     3

int ishexdigit(char c) {
   return isdigit(c) || c == 'a' || c == 'b' || c == 'c' || c == 'd' || c == 'e' || c == 'f';
}

/*
  Converts a string s to a number. Hex numbers have 0x as a prefix
 */
int number(char *s) {
    int positive = 1;
    int base = 10;
    if (s[0] == '-') {
        positive = 0;
        s++;
    }
    if (s[0] == '0' && s[1] == 'x') {
        s = s + 2;
        base = 16;
    }
    for (int i = 0; s[i] != '\0'; i++)
        if (!ishexdigit(s[i]))
            return -1;
    int ans = (int)strtol(s, NULL, base);
    return positive ? ans : -ans;
}

/*
 Print disassembled memory to stdout from start_addres to start_addres+num_words-1
 */
void memory_list(int start_address, int num_words) {
    for (int i = start_address; i < start_address+num_words; i+=4) {
        int val;
        system_bus(i, &val, READ);
        char *inst = disassemble(val);
        printres("0x%08x: 0x%08x: %s", i, val, inst);
    }
}

static char whitespace[] = " \t\r\n\v";

int cmdgetstr(char **ps, char *es, char **str) {
    char *s = *ps;
    while (s < es && strchr(whitespace, *s))
        s++;
    if (*s == 0)
        return 0;
    *str = s;
    while (s < es && !strchr(whitespace, *s))
        s++;
    *s = 0;
    *ps = s;
    //printf("getstr: %s\n", *str);
    return 1;
}

int getcmd(char *buf, int nbuf) {
    char *s = 0;
    fprintf(stderr, "chemu %% ");
    fflush(stdout);  
    memset(buf, 0, nbuf);
    s = fgets(buf, nbuf, stdin);
    if(s == 0) // EOF - enter on blank lines terminates shell
        return -1;
    buf[strcspn(buf, "\n")] = '\0';
    return 0;
}

int getscriptcmd(char *buf, int nbuf, FILE *fp) {
    char *s = 0;
    memset(buf, 0, nbuf);
    s = fgets(buf, nbuf, fp);
    if(s == 0) // EOF - enter on blank lines terminates shell
        return -1;
    buf[strcspn(buf, "\n")] = '\0';
    return 0;
}

int my_random(int start, int end) { // assumes srand has been called
    return rand() % (end-start+1) + start;
}

#define MAXARGS 10
static char *cmdargv[MAXARGS];
static int mem_dump_addr = 0;
#define MEM_SIZE_WORDS 16384 // 2 ^ 14 word addresses
#define MEM_DUMP_LEN   32
#define MEM_LIST_LEN   64
static int mem_dump_len = MEM_DUMP_LEN;
static int mem_list_addr = 0;
static int mem_list_len = MEM_LIST_LEN;

extern int breakpoint0, breakpoint1;

// Parameter finished is the status returned from step_n / step.
void step_results(enum stepret finished) {
    if (finished == BREAKPOINT) {
        if (get_reg(PC) == breakpoint0)
            printres("breakpoint: 0x%08x", breakpoint0);
        else
            printres("breakpoint: 0x%08x", breakpoint1);
    }
    else if (finished == BALTOSELF) {
        printres("branch to self:");
    }
    else if (finished == SCANF) {
        printres("<<< *** >>>");
        printres("waiting on input from scanf:");
        printres("<<< *** >>>");
    }
    else if (finished == MEMERROR) {
        int inst; 
        system_bus(get_reg(PC), &inst, READ);
        printres("Instruction mem access error: pc: 0x%08x, inst: 0x%08x", get_reg(PC), inst);
    }
    else if (finished == LOOPSTOP) {
        int inst; 
        system_bus(get_reg(PC), &inst, READ);
        printres("Executed %d instrs: pc: 0x%08x, inst: 0x%08x", loop_stop, get_reg(PC), inst);
    }
    else if (finished < 0) {
        int inst; 
        system_bus(get_reg(PC), &inst, READ);
        printres("Illegal instruction: pc: 0x%08x, inst: 0x%08x", get_reg(PC), inst);
    }

}

void get_proc_name(int proc_addr, char *proc_name) {
    int j, k, four_chars;
    for (j = 0, k = 0; j < 4; j++, k+=4) {
        system_bus(proc_addr + PROC_NAME + (j * 4), &four_chars, READ);
        proc_name[k+0] = (four_chars & 0xff000000) >> 24; // byteswap
        proc_name[k+1] = (four_chars & 0x00ff0000) >> 16;
        proc_name[k+2] = (four_chars & 0x0000ff00) >> 8;
        proc_name[k+3] = (four_chars & 0x000000ff) >> 0;
    }
}

int do_cmd(int argc, char **cmdargv) {
    enum stepret finished = NORMAL;
    int retval = 1;
    if (cmdargv[0][0] == 'r' && cmdargv[0][1] == 'u' && cmdargv[0][2] == 'n') {
        while (1) {
            finished = step_n(100);
            if (finished)
                break;
        }
        step_results(finished);
        pipeline();
    } else if (cmdargv[0][0] == 't' && cmdargv[0][1] == 'm' && cmdargv[0][2] == 'r') {
        interrupt(EXTERNRUPT);
    } else if (cmdargv[0][0] == 'r' && cmdargv[0][1] == 'k') {
        if (argc == 3) { // cmd is rk 0 55, modify kreg r0 to be 55
            int reg = number(cmdargv[1]);
            int val = 0;
            val  = number(cmdargv[2]);
            if (reg == -1 || reg > 15) {
                printres("%s", "invalid reg on rk command.");
            }
            else {
                set_kreg(reg, val);
                printres("KR%-3d:%11d (0x%.8x)", reg, get_kreg(reg), get_kreg(reg));
            }
        } else if (argc == 2) { // cmd is rk 6, display kreg r6
            int reg = number(cmdargv[1]);
            if (reg == -1 || reg > 15) {
                printres("%s", "invalid reg on rk command.");
            }
            else {
                printres("KR%-3d:%11d (0x%.8x)", reg, get_kreg(reg), get_kreg(reg));
            }
        } else { // print all kregs
            printres("r0:cpsr, r1:kpsr, r2:kr13, r3:kr14, r4:irsr, r5:ir13, r6:ir14");
        }
    } else if (cmdargv[0][0] == 'r') {
        if (argc == 3) { // cmd is r 0 55, modify r0 to be 55
            int reg = number(cmdargv[1]);
            int rval = 0, val = 0;
            if (cmdargv[2][0] == 'r') { // generate a random 32-bit value
                if (cmdargv[2][1] == '1') // random number between 0 and 100
                    rval = my_random(0, 100);
                else if (cmdargv[2][1] == '2') // random number between 0 and 200
                    rval = my_random(0, 200);
                else if (cmdargv[2][1] == '-' && cmdargv[2][2] == '1') // negative random number between 0 and 100
                    rval = -my_random(0, 100);
                else if (cmdargv[2][1] == '-' && cmdargv[2][2] == '2') // negative random number between 0 and 200
                    rval = -my_random(0, 200);
                else // 32-bit random number
                    rval = rand(); //0xfeedabee;
            }
            else
                val  = number(cmdargv[2]);
            if (reg == -1 || reg > 15) {
                printres("%s", "invalid reg on r command.");
            }
            else {
                val = rval ? rval : val;
                set_reg(reg, val);
                printres("R%-3d:%11d (0x%.8x)", reg, get_reg(reg), get_reg(reg));
            }
        } else if (argc == 2) { // cmd is r 6, display r6
            int reg = number(cmdargv[1]);
            if (reg == -1 || reg > 15) {
                printres("%s", "invalid reg on r command.");
            }
            else {
                printres("R%-3d:%11d (0x%.8x)", reg, get_reg(reg), get_reg(reg));
            }
        } else { // print all regs
#ifdef NCURSES
            printres("See regs in window above.");
#else
            printres("Regs.");
#endif
        }
    } else if (cmdargv[0][0] == 's' && cmdargv[0][1] == 't') {
        if (argc == 2) { // format is st num
            int steps = number(cmdargv[1]);
            if (steps == -1 || steps > 30000) {
                printres("%d: %s", steps, "invalid number of steps on st command - 30000 max.");
            }
            else {
                loop_stop = steps;
            }
        }
        else {
            printres("Total Steps: %d", total_steps);
        }
    } else if (cmdargv[0][0] == 's' && cmdargv[0][1] == 'o') {
        int tbp = breakpoint1;
        breakpoint1 = get_reg(PC) + 4;
        while (1) {
            finished = step_n(100);
            if (finished)
                break;
        }
        // Do not print results for a normal step over
        if (finished != BREAKPOINT || get_reg(PC) != breakpoint1)
            step_results(finished);
        pipeline();
        breakpoint1 = tbp;
    } else if (cmdargv[0][0] == 's') {
        if (argc == 2) { // format is s num
            int steps = number(cmdargv[1]);
            if (steps == -1 || steps > 30000) {
                printres("%d: %s", steps, "invalid number of steps on s command - 30000 max.");
            }
            else {
                finished = step_n(steps);
                step_results(finished);
                pipeline();
            }
        }
        else {
            finished = step();
            step_results(finished);
            pipeline();
            show_mem_changed();
        }
    } else if (cmdargv[0][0] == 'd') {
        int t = 0, mem_dump_inc = 0;
        if (argc > 1) { // at least d addr or d symbol
            if (cmdargv[1][0] >= 'a' && cmdargv[1][0] <= 'z') { // d symbol
                t = dictget(cmdargv[1]);
                mem_dump_addr = t != -1000001 ? t : mem_dump_addr;
            }
            else { // d addr
                t = number(cmdargv[1]);
                mem_dump_addr = t >= 0 ? t : mem_dump_addr;
            }
        }
        if (argc > 2) { // we have d addr len or d symbol len
            t = number(cmdargv[2]);
            mem_dump_len = t >= 0 ? t : mem_dump_len;
        }
        if (cmdargv[0][1] == 'b') {
            if (dump_memory(mem_dump_addr, mem_dump_len)) {
                printres("%s", "Invalid address");
            } 
            mem_dump_inc = mem_dump_len;
        }
        else {
            if (dump_memory_word(mem_dump_addr, mem_dump_len)) {
                printres("%s", "Invalid address");
            }
            mem_dump_inc = mem_dump_len * 4;
        }
        mem_dump_addr += mem_dump_inc;
    } else if (cmdargv[0][0] == 'l' && cmdargv[0][1] == 'd') {
      if (argc != 2) { // must issue ld filename
          printres("%s", "format is ld filename");
      } else {
          load_memory(cmdargv[1]);
      }
    } else if (cmdargv[0][0] == 'l') {
        int t = 0;
        if (argc > 1) { // at least l addr or l symbol
            if (cmdargv[1][0] == 'p' && cmdargv[1][1] == 'c')
                t = get_reg(15);
            else if (cmdargv[1][0] >= 'a' && cmdargv[1][0] <= 'z') { // l symbol
                t = dictget(cmdargv[1]); // returns -1000001 if symbol not found
            }
            else
                t = number(cmdargv[1]);
            mem_list_addr = t >= 0 ? t : mem_list_addr;
        }
        if (argc > 2) { // we have l addr len or l symbol len
            t = number(cmdargv[2]);
            mem_list_len = t >= 0 ? t*4 : mem_list_len;
        }
        memory_list(mem_list_addr, mem_list_len);
        mem_list_addr += mem_list_len;
    } else if (cmdargv[0][0] == 'm' && cmdargv[0][1] == 'b') {
        if (argc == 3) { // format is mb addr value
            int addr; 
            unsigned char val;
            if (cmdargv[1][0] >= 'a' && cmdargv[1][0] <= 'z') // m symbol val
                addr = dictget(cmdargv[1]);
            else // m addr val
                addr = number(cmdargv[1]);

            if ((cmdargv[2][0] >= 'a' && cmdargv[2][0] <= 'z') ||   // m addr letter
                (cmdargv[2][0] >= 'A' && cmdargv[2][0] <= 'Z'))     // m addr letter
                val = cmdargv[2][0];
            else // m addr val
                val  = number(cmdargv[2]);
            if (addr < 0) {
                printres("%s", "invalid address on m command.");
            }
            else {
                system_bus_b(addr, &val, WRITE);
            }
        }
        else {
            printres("%s", "format is mb addr value");
        }
    } else if (cmdargv[0][0] == 'm') {
        if (argc == 3) { // format is m addr value
            int addr;
            if (cmdargv[1][0] >= 'a' && cmdargv[1][0] <= 'z') // m symbol val
                addr = dictget(cmdargv[1]);
            else // m addr val
                addr = number(cmdargv[1]);

            int rval = 0, val = 0;
            if (cmdargv[2][0] == 'r') { // generate a random 32-bit value
                if (cmdargv[2][1] == '1') // random number between 0 and 100
                    rval = my_random(0, 100);
                else if (cmdargv[2][1] == '2') // random number between 0 and 200
                    rval = my_random(0, 200);
                else if (cmdargv[2][1] == '-' && cmdargv[2][2] == '1') // negative random number between 0 and 100
                    rval = -my_random(0, 100);
                else if (cmdargv[2][1] == '-' && cmdargv[2][2] == '2') // negative random number between 0 and 200
                    rval = -my_random(0, 200);
                else // 32-bit random number
                    rval = rand(); //0xfeedabee;
            }
            else
                val  = number(cmdargv[2]);
            if (addr < 0) {
                printres("%s", "invalid address on m command.");
            }
            else {
                val = rval ? rval : val;
                system_bus(addr, &val, WRITE);
            }
        }
        else {
            printres("%s", "format is m addr value");
        }
    } else if (cmdargv[0][0] == 'p' && cmdargv[0][1] == 'l') {
        pipeline();
      // ps command: prints out info of ptable
    } else if (cmdargv[0][0] == 'p' && cmdargv[0][1] == 's') {
        int table_addr = dictget("ptable"); // check if ptable label exists
        if (table_addr == -1000001) {
            printres("%s", "cannot find ptable in memory!");
            return retval;
        }
        
        int proc_addr, pid, state, start, ustack, kstack, context, trapframe, procpc;
        char proc_name[16] = { 0 };
        char *state_to_str[] = {"rdy  ", "run  ", "oth  "};

        if (argc == 1) { // ps without args, show all with state != 0
            printres("%3s %16s %5s", "PID", "Name", "State");
            for (int i = 0; i < 8; i++) { // currently we have upto 8 procs
                proc_addr = table_addr + (i * PROC_SIZE);
                system_bus(proc_addr + PROC_STATE, &state, READ);
                if (state == 0) // 0 indicates unused proc
                    continue;
                else if (state == STATE_READY || state == STATE_RUN) // ready or run
                    state = state - 2; // adjust to index int state_to_str
                else
                    state = 2;         // select "oth" in state_to_str
                system_bus(proc_addr, &pid, READ);   // pid is first entry in proc in ptable
                get_proc_name(proc_addr, proc_name); // get proc_name from proc in ptable
                printres("%3u %16s %s", pid, (char*)proc_name, state_to_str[state]);
            }
        }
        else { // ps <pid> or ps <pname>
            int ps_pid = 0;
            if (argc > 1 && cmdargv[1][0] > '0' && cmdargv[1][0] < '9') // ps <pid>
                ps_pid = number(cmdargv[1]);
            bool found = false;
            for (int i = 0; i < 8; i++) { // currently we have upto 8 procs
                proc_addr = table_addr + (i * PROC_SIZE);
                get_proc_name(proc_addr, proc_name); // get proc_name from ptable
                system_bus(proc_addr, &pid, READ); // pid is first entry in proc
                if (ps_pid != 0 && pid == ps_pid) // ps <pid>
                    found = true;
                else if (strcmp(cmdargv[1], (char *)proc_name) == 0) // ps <name>
                    found = true;
                if (found) {
                    system_bus(proc_addr + PROC_STATE, &state, READ);
                    system_bus(proc_addr + PROC_START, &start, READ);
                    system_bus(proc_addr + PROC_USTACK, &ustack, READ);
                    system_bus(proc_addr + PROC_KSTACK, &kstack, READ);
                    system_bus(proc_addr + PROC_CONTEXT, &context, READ);
                    system_bus(proc_addr + PROC_TRAP, &trapframe, READ);
                    if (state == STATE_RUN)
                        procpc = get_reg(PC);
                    else
                        system_bus(trapframe + TF_PC, &procpc, READ);
                    if (state == STATE_READY || state == STATE_RUN)
                        state = state - 2;
                    else
                        state = 2;
                    printres("PID             Name State Ustk Kstk Cont TrFr   PC");
                    printres("%3u %16s %s %4x %4x %4x %4x %4x", pid, (char*)proc_name, state_to_str[state], ustack, kstack, context, trapframe, procpc);
                    found = true;
                    break;
                }
            }
            if (!found) {
                if (ps_pid)
                    printres("%s%d", "cannot find process with pid: ", ps_pid);
                else
                    printres("%s%s", "cannot find process with name: ", cmdargv[1]);
            }
        }
    } else if (cmdargv[0][0] == 'b') {
        int *bp = &breakpoint0;
        if (cmdargv[0][1] == '1')
            bp = &breakpoint1;
        if (argc > 1) { // b addr - set a break point
            int addr;
            if (cmdargv[1][0] >= 'a' && cmdargv[1][0] <= 'z') // b symbol
                addr = dictget(cmdargv[1]);
            else // b addr
                addr = number(cmdargv[1]);
            if (addr < 0)
                *bp = -1;
            else
                *bp = addr;
        } else {
            printres("breakpoint 0: 0x%08x", breakpoint0);
            printres("breakpoint 1: 0x%08x", breakpoint1);
        }
    } else if (cmdargv[0][0] == 'c' && cmdargv[0][1] == 'p') {
      if (argc != 3) {
          printres("%s", "format is cp addr string");
      }
      else {
          int addr = number(cmdargv[1]);
          if (addr == -1 || addr > MEM_SIZE_WORDS-mem_list_len) {
             printres("%s", "invalid addr on cp command.");
          }
          else { // TODO Update to be cp 0x100 "Gusty Cooper"  New is put string in "" 
              char chars[64] = {0}; 
              strcpy((char *)chars, cmdargv[2]);
              int sl = strlen(cmdargv[2]);
              if (sl % 4 == 0) // strlen is multiple of 4
                  sl++;
              for (int i = 0; i < sl; i+=4) {
                  int val = chars[i] << 24 | chars[i+1] << 16 | chars[i+2] << 8 | chars[i+3];
                  //printf("s: %s, val: %x, chars: %s\n", cmdargv[2], val, chars);
                  system_bus(addr, &val, WRITE);
                  addr += 4;
              }
          }
      } 
    } else if (cmdargv[0][0] == '0' && cmdargv[0][1] == 'x' ) {
        int val = (int)strtol(&cmdargv[0][2], NULL, 16);
        printres("decimal: %d", val);
    } else if (cmdargv[0][0] == '0' && cmdargv[0][1] == 'd' ) {
        int val = (int)strtol(&cmdargv[0][2], NULL, 10);
        printres("hex: 0x%x", val);
    } else if (cmdargv[0][0] == 'y') { // show symbols
        dictshow();
    } else if (cmdargv[0][0] == '.') {
        for (int i = 0; i < strlen(cmdargv[0]); i++) // cp the null char also
            cmdargv[0][i] = cmdargv[0][i+1];
        if (fork() == 0) 
            execvp(cmdargv[0], cmdargv);
        wait(NULL);
    } else if (cmdargv[0][0] == 'v') {
        if (argc > 1)
            verbose_cpu = 1;
        else
            verbose_cpu = 0;

    } else if (cmdargv[0][0] == 'q') {
        retval = 0;
    }
    else {
        printres("%s", "Invalid command.");
    }
    return retval;
}

void do_script(char *scriptfilename) {
    printres("< redirect from script. file: %s\n", scriptfilename);
    cmdargv[0] = ""; // prevent seg fault when first cmd is empty line
    FILE *fp = fopen(scriptfilename, "r");
    if (fp != NULL) {
        char command[100];
        while (getscriptcmd(command, sizeof(command), fp) >= 0) {
            char *str;
            char *s = command;
            char *es = s + strlen(s);
            int argc = 0;
            cmdargv[0] = "";
            while (cmdgetstr(&s, es, &str) != 0) {
                cmdargv[argc] = str;
                //printf("%s\n", cmdargv[argc]);
                argc++;
            }
            cmdargv[argc] = 0;
            if (argc > 0) {
                if (cmdargv[0][0] != '<') {
                    if (do_cmd(argc, cmdargv) == 0)
                        break;
                }
            }
        }
    }
    else {
      printf("File %s not found.\n", cmdargv[1]);
    }
}

/*
  $ emu code.o : run emu and load code.o at addresses specified in code.o
  $ emu -s script.txt code.o : run emu, load code.o at addr speed in code.o, run script.txt
  $ emu -o code.o : run emu, load OS, load code.o at addresses specified, SP is set 
  $ emu -o code.o arg1 arg2 : run emu, load os, load code.o at address 0, arg1/2 passed on stack
  $ emu --OS newos.o code.o arg1 arg2 : like prev, but newos.o is loaded
  -o or --os : load OS as part of emu, os is in os.o
  -f file.o or --OS file.o : load OS as part of emu. OS in file.o
 */
static struct option long_options[] = {
  {"white",  no_argument,       0,  'w' },
  {"ms",     no_argument,       0,  'm' },
  {"os",     no_argument,       0,  'o' },
  {"OS",     required_argument, 0,  'f' },
  {"script", required_argument, 0,  's' },
  {0,        0,                 0,   0  }
};

static char *osfilename = "no os";
static char *scriptfilename = "no script";
/*
 * process_args - called to process command line args
 * argc and argv - propogated from main()
 * return 0  - success, black background
 * return 1  - success, white background
 *        -1 - error
 */
int process_args(int argc, char **argv) {
    int load_os = 0, load_os2nd = 0, use_script = 0, rc = 0; // rc = 0 is black background
    int opt, long_index;
    while ((opt = getopt_long(argc, argv, "wmof:s:", long_options, &long_index )) != -1) {
        switch (opt) {
          case 'w': 
            rc = 1; // white background
            break;
          case 'm': 
            load_os2nd = 1;
            osfilename = "os.o";
            break;
          case 'o': 
            load_os = 1;
            osfilename = "os.o";
            break;
          case 'f': 
            load_os = 1;
            osfilename = optarg; 
            break;
          case 's':
             use_script = 1;
             scriptfilename = optarg;
             break;
          default: 
            printf("Invalid invocation.\n");
            return -1;
        }
    }
    argc -= optind;
    argv += optind;
    printres("load os: %d", load_os);
    printres("os filename: %s", osfilename);
    printres(".o filename: %s", argv[0]);
    printres("scriptfilename: %s", scriptfilename);
    if(!argv[0]) {
        fprintf(stderr, "%s optind: %d argc: %d\n", "Wrong number of arguments", optind, argc);
        return -1;
    }
    if (load_os) {
        FILE *osf = fopen(osfilename, "rb");
        if (osf == NULL) {
            fprintf(stderr, "File not found: %s\n", osfilename==NULL ? "os filename" : osfilename);
            return -1;
        }
        fclose(osf);
        load_memory(osfilename);
        set_cpsr(OS);
        //set_cpsr(U);
        //set_rupt(0x7ff0);
        set_rupt(0xff00);
    }
    FILE *file = fopen(argv[0], "rb");
    if (file == NULL) {
        fprintf(stderr, "File not found: %s\n", argv[0]);
        return -1;
    }
    fclose(file);
    printres("loading %s", argv[0]);
    load_memory(argv[0]); // load user program
    if (load_os2nd) {
        FILE *osf = fopen(osfilename, "rb");
        if (osf == NULL) {
            fprintf(stderr, "File not found: %s\n", osfilename==NULL ? "os filename" : osfilename);
            return -1;
        }
        fclose(osf);
        load_memory(osfilename);
        set_cpsr(OS);
        //set_cpsr(U);
        //set_rupt(0x7ff0);
        set_rupt(0xff00);
    }
    if (use_script) {
        do_script(scriptfilename);
    }
    srand(time(0)); // initialize for rand() to work
    return rc;
}
