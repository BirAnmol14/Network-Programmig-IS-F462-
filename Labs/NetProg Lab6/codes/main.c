#include <stdio.h>
#include "checkPrime.h"
#include "findGCD.h"
int main(){
	int n = 0;
	int option;
	puts("Enter option(1. gcd\n2. CheckPrime): ");
	scanf("%d",&option);
	if(option ==1){
		int a,b;
		puts("Enter number a");
		scanf("%d",&a);
		puts("Enter Number b");
		scanf("%d",&b);
		printf("GCD of %d and %d id %d\n",a,b,findGCD(a,b));
		return 0;
	}else{
	printf("Enter Number\n");
	scanf("%d",&n);
	if(checkPrime(n) == 0){
		printf("%d is not Prime\n",n);
	}else{
		printf("%d is Prime\n",n);
	}
	return 0;
	}
}
