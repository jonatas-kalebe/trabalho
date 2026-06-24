#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Mini-simulador MIPS para o SUBCONJUNTO de instrucoes geradas pelo nosso
# compilador. NAO e um simulador completo -- serve apenas para VALIDAR que o
# codigo gerado executa e produz a saida correta (alternativa ao SPIM/MARS).
#
# Uso:  python sim.py programa.asm  [valores_de_entrada_separados_por_espaco]
import sys, re

def parse_mem_operand(tok):
    # "4($sp)" -> (4, "$sp")
    m = re.match(r'(-?\d+)\(([^)]+)\)', tok)
    return int(m.group(1)), m.group(2)

def main():
    path = sys.argv[1]
    inputs = []
    if len(sys.argv) > 2:
        inputs = sys.argv[2].split()
    inp_idx = [0]
    def read_int():
        v = int(inputs[inp_idx[0]]); inp_idx[0]+=1; return v

    lines = open(path, encoding='utf-8').read().splitlines()
    data_strings = {}        # label -> texto
    str_addr = {}            # label -> endereco fake
    addr_str = {}            # endereco -> texto
    next_str_addr = 0x10000000
    insns = []               # lista de (op, [args])
    labels = {}              # label -> indice da instrucao
    section = None
    for ln in lines:
        s = ln.strip()
        if not s: continue
        if s == '.data': section='data'; continue
        if s == '.text': section='text'; continue
        if s.startswith('.globl'): continue
        if section=='data':
            m = re.match(r'(\w+):\s*\.asciiz\s*"(.*)"\s*$', s)
            if m:
                lab=m.group(1); raw=m.group(2)
                txt=raw.encode().decode('unicode_escape')
                data_strings[lab]=txt
                str_addr[lab]=next_str_addr
                addr_str[next_str_addr]=txt
                next_str_addr+=0x1000
            continue
        # section text
        if s.endswith(':'):
            labels[s[:-1]] = len(insns)
            continue
        # instrucao
        parts = s.split(None,1)
        op = parts[0]
        args = []
        if len(parts)>1:
            args = [a.strip() for a in parts[1].split(',')]
        insns.append((op,args))

    reg = {}
    def R(r): return 0 if r=='$zero' else reg.get(r,0)
    def setR(r,v):
        if r!='$zero': reg[r]=v
    mem = {}
    STACK_TOP = 0x00080000
    reg['$sp']=STACK_TOP
    out=[]

    def val(tok):
        # registrador, label de string, ou imediato
        if tok.startswith('$'): return R(tok)
        if tok in str_addr: return str_addr[tok]
        return int(tok)

    pc=0
    steps=0
    while 0<=pc<len(insns):
        steps+=1
        if steps>5_000_000:
            out.append("\n[ABORTADO: limite de passos]"); break
        op,args=insns[pc]; pc+=1
        if op=='li':   setR(args[0], val(args[1]))
        elif op=='la': setR(args[0], val(args[1]))
        elif op=='move': setR(args[0], R(args[1]))
        elif op=='addiu': setR(args[0], R(args[1])+int(args[2]))
        elif op=='add': setR(args[0], R(args[1])+R(args[2]))
        elif op=='sub': setR(args[0], R(args[1])-R(args[2]))
        elif op=='mul': setR(args[0], R(args[1])*R(args[2]))
        elif op=='div':
            a=R(args[1]); b=R(args[2]); q=abs(a)//abs(b)
            if (a<0)!=(b<0): q=-q
            setR(args[0], q)
        elif op=='sll': setR(args[0], R(args[1])<<int(args[2]))
        elif op=='and': setR(args[0], R(args[1]) & R(args[2]))
        elif op=='or':  setR(args[0], R(args[1]) | R(args[2]))
        elif op=='slt': setR(args[0], 1 if R(args[1])<R(args[2]) else 0)
        elif op=='sle': setR(args[0], 1 if R(args[1])<=R(args[2]) else 0)
        elif op=='sgt': setR(args[0], 1 if R(args[1])>R(args[2]) else 0)
        elif op=='sge': setR(args[0], 1 if R(args[1])>=R(args[2]) else 0)
        elif op=='seq': setR(args[0], 1 if R(args[1])==R(args[2]) else 0)
        elif op=='sne': setR(args[0], 1 if R(args[1])!=R(args[2]) else 0)
        elif op=='lw':
            off,base=parse_mem_operand(args[1]); setR(args[0], mem.get(R(base)+off,0))
        elif op=='sw':
            off,base=parse_mem_operand(args[1]); mem[R(base)+off]=R(args[0])
        elif op=='beq':
            if R(args[0])==R(args[1]): pc=labels[args[2]]
        elif op=='bne':
            if R(args[0])!=R(args[1]): pc=labels[args[2]]
        elif op=='b' or op=='j': pc=labels[args[0]]
        elif op=='jal': setR('$ra', pc); pc=labels[args[0]]
        elif op=='jr':  pc=R(args[0])
        elif op=='syscall':
            code=R('$v0')
            if code==1:   out.append(str(R('$a0')))
            elif code==4: out.append(addr_str.get(R('$a0'),''))
            elif code==11:out.append(chr(R('$a0')&0xff))
            elif code==5: setR('$v0', read_int())
            elif code==12:setR('$v0', read_int())
            elif code==10: break
            else: out.append(f"[syscall {code}?]")
        else:
            out.append(f"[op desconhecida: {op}]")
    sys.stdout.write(''.join(out))

if __name__=='__main__':
    main()
