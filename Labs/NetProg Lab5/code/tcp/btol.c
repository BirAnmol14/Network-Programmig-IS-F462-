#include <stdio.h>
#include <stdlib.h>
int btol(int num){
	//converts little endian integer to big endian
	int byte0, byte1, byte2, byte3;
	   byte0 = (num & 0x000000FF) >> 0 ;
	   byte1 = (num & 0x0000FF00) >> 8 ;
	   byte2 = (num & 0x00FF0000) >> 16 ;
	   byte3 = (num & 0xFF000000) >> 24 ;
	   return((byte0 << 24) | (byte1 << 16) | (byte2 << 8) | (byte3 << 0));
}
int main(){
	int n = 0x12345678; //Hexadecimal of two bytes
	//this is little endian
	printf("Little endian number: 0x%x(%d) -> big endian: 0x%x(%d)\n",n,n,btol(n),btol(n));
}
