#include "findGCD.h"
int findGCD(int x,int y){
	if(x==y){
		return y;
	}
	if(x>y){
		int gcd = y;
		while(x%y !=0){
			gcd=x%y;
			x=y;
			y=gcd;
		}
		return gcd;
	}
	else{
		int gcd = x;
		while(y%x !=0){
			gcd=y%x;
			y=x;
			x=gcd;
		}
		return gcd;
	}
}
