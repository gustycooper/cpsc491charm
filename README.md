# Charm
Charm consists of several components. You can read more at *[Charm Guide](https://gusty.bike/charm.html)*.
* Charm Instruction Set Architecture - definition of registers, register conventions, instructions, and instruction format.
* Charm Assembly Language - definition of an assembly language programmers can use to write Charm assembly programs.
* Charm Assembler - chasm - an assembler that translates Charm assembly language to a file format compatible with the Charm Emulator.
* Charm Emulator - chemu - An emulation of the Charm ISA. The emulation consists of one CPU and main memory. The output of chasm can be loaded into chemu and executed. There are two versions of chemu.
  * chemut - Emulation uses stdin, stdout, and stderr in a terminal for control and display.
  * chemun - Emulation uses ncurses to create control and display windows in the terminal.
* Charm C Compiler - a compiler that translates C into Charm assembly. I modified [Rui Ueyamm's chibicc](https://github.com/rui314/chibicc) to generate Charm Assembly.
* Charm Linker - a linker that links multiple Charm .o files into a_out.o.

This version of Charm is developing an OS that manages processes. 
* Matt Nguyen and Gusty are working on this.
* We only have chasm and chemu.
* We do not have chibicc at this time.
