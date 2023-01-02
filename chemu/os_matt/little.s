// processes gusty, lauren, and matt do the following
// char name_str[] = "gusty";
// int main() {
//   char name[8];
//   strcpy(name, name_str);
//   int i = 1;
//   while (i) {
//       i++;
//   }
// }
// stack
// 00 gust
// 04 y000
// 08 i's value
// 0c lr's value

// Define OS API adresses
.text 0xa000
.label printf
.text 0xa004
.label scanf
.text 0xa008
.label yield
.text 0xa00c
.label strcpy
.text 0xa010
.label sleep
.text 0xa014
.label wake
.text 0xa018
.label fork
.text 0xa01c
.label exec

.data 0x0200
.label gusty_str
.string //gusty
.label fmt
.string //%s: %d
.label scanfmt
.string //%d
.label xvar
0
.label loadme
.string //loadme.o
.text 0x0300
.label gusty
sub sp, sp, 32
str lr, [sp, 12]
mov r2, 1
str r2, [sp, 8] // i = 1
mva r1, gusty_str
mov r0, sp
blr strcpy
ldr r2, [sp, 8] // i to r2
mva r1, fmt
mva r2, gusty_str
mov r3, 777
ker 0x11
mva r0, fmt
mva r1, gusty_str
mov r2, 222
blr printf
mva r1, fmt
mva r2, gusty_str
mov r3, 888
ker 0x11
//mva r0, scanfmt // uncomment to read int into xvar
//mva r1, xvar
//blr scanf
mva r0, loadme
blr exec          // exec loads a .o - does not start a proc
.label after_exec
mov r0, 0x55      // sleep on channel 0x55
blr sleep
.label before_yield
blr yield
.label loop_gusty
cmp r2, 0
beq end_gusty
//blr yield
add r2, r2, 1
str r2, [sp, 0] // i++
bal loop_gusty
.label end_gusty
ldr lr, [sp, 4]
sub sp, sp, 32
mov r15, r14

.data 0x0400
.label lauren_str
.string //lauren
.text 0x0500
.label lauren
sub sp, sp, 32
str lr, [sp, 12]
mov r2, 1
str r2, [sp, 8] // i = 1
mva r1, lauren_str
mov r0, sp
blr strcpy
ldr r2, [sp, 8] // i to r2
mva r0, fmt
mva r1, lauren_str
mov r2, 333
blr printf
mov r0, 0x55
blr wake
.label loop_lauren
cmp r2, 0
beq end_lauren
add r2, r2, 1
str r2, [sp, 0] // i++
bal loop_lauren
.label end_lauren
ldr lr, [sp, 4]
sub sp, sp, 32
mov r15, r14

.data 0x0600
.label matt_str
.string //matt
.text 0x0700
.label matt
sub sp, sp, 32
str lr, [sp, 12]
mov r2, 1
str r2, [sp, 8] // i = 1
mva r1, matt_str
mov r0, sp
blr strcpy
ldr r2, [sp, 8] // i to r2
.label loop_matt
cmp r2, 0
beq end_matt
add r2, r2, 1
str r2, [sp, 0] // i++
bal loop_matt
.label end_matt
ldr lr, [sp, 4]
sub sp, sp, 32
mov r15, r14

.stack 0x0e00

