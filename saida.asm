    .data
str0: .asciiz "digite um numero"
str1: .asciiz "O fatorial de "
str2: .asciiz " e: "
    .text
    .globl main
main:
    move $s1, $sp
    addiu $sp, $sp, -4
    move $fp, $sp
    addiu $sp, $sp, -4
    li $s0, 1
    sw $s0, 0($s1)
while_0:
    lw $s0, 0($s1)
    sw $s0, 0($sp)
    addiu $sp, $sp, -4
    li $s0, 0
    lw $t1, 4($sp)
    addiu $sp, $sp, 4
    sgt $s0, $t1, $s0
    beq $s0, $zero, endwhile_0
    la $a0, str0
    li $v0, 4
    syscall
    li $a0, 10
    li $v0, 11
    syscall
    li $v0, 5
    syscall
    sw $v0, 0($s1)
    b while_0
endwhile_0:
    la $a0, str1
    li $v0, 4
    syscall
    lw $s0, 0($s1)
    move $a0, $s0
    li $v0, 1
    syscall
    la $a0, str2
    li $v0, 4
    syscall
    sw $fp, 0($sp)
    addiu $sp, $sp, -4
    lw $s0, 0($s1)
    sw $s0, 0($sp)
    addiu $sp, $sp, -4
    jal fatorial
    move $a0, $s0
    li $v0, 1
    syscall
    li $a0, 10
    li $v0, 11
    syscall
main__fim:
    li $v0, 10
    syscall

fatorial:
    move $fp, $sp
    sw $ra, 0($sp)
    addiu $sp, $sp, -4
    lw $s0, 4($fp)
    sw $s0, 0($sp)
    addiu $sp, $sp, -4
    li $s0, 0
    lw $t1, 4($sp)
    addiu $sp, $sp, 4
    seq $s0, $t1, $s0
    beq $s0, $zero, else_1
    li $s0, 1
    b fatorial__epi
    b endif_1
else_1:
    lw $s0, 4($fp)
    sw $s0, 0($sp)
    addiu $sp, $sp, -4
    sw $fp, 0($sp)
    addiu $sp, $sp, -4
    lw $s0, 4($fp)
    sw $s0, 0($sp)
    addiu $sp, $sp, -4
    li $s0, 1
    lw $t1, 4($sp)
    addiu $sp, $sp, 4
    sub $s0, $t1, $s0
    sw $s0, 0($sp)
    addiu $sp, $sp, -4
    jal fatorial
    lw $t1, 4($sp)
    addiu $sp, $sp, 4
    mul $s0, $t1, $s0
    b fatorial__epi
endif_1:
fatorial__epi:
    lw $ra, 0($fp)
    move $sp, $fp
    addiu $sp, $sp, 8
    lw $fp, 0($sp)
    jr $ra

