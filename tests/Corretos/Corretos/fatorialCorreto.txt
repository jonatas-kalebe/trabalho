global [
	n: int;
]
/* Este e um programa CORRETO em Cafezinho que calcula o fatorial de um nmero*/

funcao[
		fatorial(n:int): int {
			se (n==0)
			entao
				retorne 1;
			senao
				retorne n* fatorial(n-1);
			fimse	
		}
]

principal {
	n =1;
	enquanto (n>0)  {
       		escreva "digite um numero";
       		novalinha;
       		leia n;    
	}	
	escreva "O fatorial de ";
	escreva n;
	escreva " e: ";
	escreva fatorial(n);
	novalinha;
}
