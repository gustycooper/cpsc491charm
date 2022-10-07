// processes gusty and lauren both do the following
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

.text 0x0100
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

.data 0x0200
.label gusty_str
.string //gusty
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
.label loop_gusty
cmp r2, 0
beq end_gusty
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
mva r1, gusty_str
mov r0, sp
blr strcpy
ldr r2, [sp, 8] // i to r2
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

.stack 0x0e00

