principal 
/*Esse programa pede, recursivamente, o tamanho de uma sequencia de numeros inteiros.
Se for digitado o valor zero o programa termina. Caso contrário ele pede para o usario digitar
uma squência de números inteiros de tamanho igual ao tamanho especificado. Em seguida, o programa
imprime uma mensagem indicando de a sequência foi digitada em ordem crescente ou não. */

	{
		quant: int;
	}
	{
		escreva "digite o tamanho de uma sequencia de numeros inteiros - digite 0 para terminar.";
		leia quant;
		enquanto (quant != 0)
		{
			cont, valAtual:int;
			ordenado:car;
		}
		{	
			ordenado='v';
			cont=1;
			leia valAtual;
			escreva "digite uma sequencia de ";
			escreva quant;
			escreva " numeros inteiros separados entre si por um espaco";
			enquanto (cont < quant)
			{
				proxVal: int; /*Variavel declarada dentro de bloco */
			}
			{
				leia proxVal;
				se (valAtual < proxVal)
				entao 
					valAtual=proxVal;
				senao
				{
					ordenado='f';
					cont=quant;
				}
				fimse
				cont=cont+1;		
			}
			se (ordenado=='v')
			entao{ 
				escreva "ORDENADA";
				novalinha;
			}
			senao{
				escreva "DESORDENADA";
				novalinha;
			}
			fimse
			leia quant;	
		}
	}
	 

