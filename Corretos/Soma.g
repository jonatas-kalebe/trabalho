principal 
	{
		a1,r,n:int;
	}
	{
		escreva "Digite o valor do termo inical da progressao aritmetica";
		novalinha;
		leia a1;
		escreva "Digite  o número de elementos da progressao artimetica";
		novalinha;
		leia n;
		escreva "Digite a razao da progressao";
		novalinha;
		leia r;
		{
			soma,i,an: int;
		}
		{  
			soma=a1;
			i=1;
			an=a1;
			enquanto (i<n)
			{
				an=an+r;
				i=i+1;
				soma = soma + an;
			}
			escreva "O valor da soma da progressao aritmetica e: ";
			escreva soma;
			novalinha;
		}				
	}
	 

