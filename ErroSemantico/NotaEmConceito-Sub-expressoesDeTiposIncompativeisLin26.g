principal 
/*Esse programa computa a soma de uma progressao aritmetica. O programa pede ao usuario
que digite o valor do termo inicial da progressao, o numero de elementos da progressao, e o valor
da razao. Em seguida computa e imprime a soma dos elementos da progresao */

	{
		nota: int;
	}
	{
		escreva "Digite um valor inteiro para a nota de um aluno";
		novalinha;
		leia nota;
		se (nota<6)
		entao
		{
			conceito: car;
		}
		{	
			conceito='D';
			escreva "Conceito: ";
			escreva conceito;
			novalinha;
		}
		senao
		{
			se(nota<'7')
			entao
			{
				conceito:car;
			}
			{
				conceito='C';
				escreva "Conceito: ";
				escreva conceito;
			    novalinha;
			}
			senao
			{
				se (nota<9)
				entao
				{
					conceito:car;
				}
				{
					conceito='B';
					escreva "Conceito: ";
					escreva conceito;
					novalinha;
				}
				senao
				{
					conceito:car;
				}
				{
					conceito='A';
					escreva "Conceito: ";
					escreva conceito;
					novalinha;
				}
				fimse
			}
			fimse
		}
		fimse
		
	}
