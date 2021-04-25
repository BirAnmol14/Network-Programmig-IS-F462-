#include <stdio.h>
#include <math.h>
#include "checkPrime.h"

int checkPrime(int n){
	//printf("Inside Check Prime\n");
	if(n==1 || n==0){
		return 1;
	}
	for(int i = 2;i <= sqrt(n);i++){
		if(n%i == 0){
			return 0;
		}
	}
	return 1;
}
