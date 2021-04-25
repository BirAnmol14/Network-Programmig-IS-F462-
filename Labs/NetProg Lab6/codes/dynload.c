#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>

int main(int argc,char *argv[]){
	void *libHandle;
	void (*funcp)(void);
	const char *err;

	if(argc != 3 || strcmp(argv[1],"--help") == 0){
		printf("%s lib-path func-name\n",argv[0]);
		exit(-1);
	}
	
	libHandle = dlopen(argv[1],RTLD_LAZY);
	if(libHandle == NULL){
		printf("dlopen: %s",dlerror());
		exit(-1);
	}

	(void)dlerror();
	*(void **)(&funcp) = dlsym(libHandle,argv[2]);
	err = dlerror();
	
	if(err != NULL){
		printf("dlsym: %s",err);
		exit(-1);
	}

	if(funcp == NULL)
		printf("%s is NULL\n",argv[2]);
	else
		(*funcp)();

	dlclose(libHandle);
	exit(0);
	return 0;
}
