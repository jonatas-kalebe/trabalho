principal 
/* Esse programa computa o fatorial de um numero inteiro positivo. O usuario deve digitar o valor
de um numero. Enquanto um valor negativo for digitado o programa emite uma mensagerm requerendo
que o usuário digite um numero nao negativo. Quando o usuario digitar um numero nao negativo,
o programa computa e emite o valor do fatorial desse numero.  */

	{
		fatorial,i,n:int;
		fatorial:int;
	}
	{
		n=-1;
		enquanto (n<=0)
			{
				escreva "Digite um numero inteiro nao negativo";
				novalinha;
				leia n;
			}
		fatorial=n;
		i=n-1;
		enquanto(i>1) 
			{
				fatorial = fatorial * i;
				i=i-1;
			}
		escreva "O fatorial de ";
		escreva n;
		escreva " e: ";
		escreva fatorial;
		novalinha;
	}
	 

