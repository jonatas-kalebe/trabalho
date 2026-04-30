principal 
/*Este programa calcula o valor do n-esimo termos de uma progressao aritmetica. 
O programa pede ao usuário o valor do termo inicial da progressao, a posicao n do termo e o
valor da razao da progressao.*/
	{
		a1,n,r:int;
	}
	{
		escreva "Digite o valor do termo inical da progressao aritmetica";
		novalinha;
		leia a1;
		escreva "Digite a posicao do elemento da progressao que se deseja obter o seu valor";
		novalinha;
		leia n;
		escreva "Digite a razao da progressao";
		novalinha;
		leia r;
		{
			i,an: int;
		}
		{
			i=1;
			an=a1;
			enquanto (i<n)
			{
				an=an+r;
				i=i+1;
			}
			escreva "O valor do elemento ";
			escreva n;
			escreva " da progressao e: ";
			escreva an;
			novalinha;
		}
		
		
	}
	 

