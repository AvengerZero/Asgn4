//header: include
#include <stdio.h>
#include <string.h>
#include <errno.h>	
#include <stdlib.h>	

//static variable declaration
static int v=0,S=0;
static char* tarfile;
static char* path;
int main(int argc, char[]* argv){
    if (argc!=3 ||argc!=4){
        printf("mytar [ctxvS]f tarfile [ path [ ... ] ]");
        exit(1);
    }
    char *a=char[1]; //for checking each letters
    int c=0,t=0,x=0,f=0;
    while (;a!='/0';a++){
        if (strcmp(a,'c')==0){c=1;}
        if (strcmp(a,'t')==0){t=1;}
        if (strcmp(a,'x')==0){x=1;}
        if (strcmp(a,'f')==0){f=1;}
    }
    if (f==0||c+t+x!=1){
        printf("mytar [ctxvS]f tarfile [ path [ ... ] ]");
        exit(1);
    }
    tarfile=argv[2];
    if (argc==4){path=argv[3];}
    else {path=NULL;}
    if (c==1){printf(creation(tarfile,path,v));} //printf as placeholder
    if (t==1){printf(listing(tarfile,v));}
    if (x==1){printf(extraction(tarfile,path));}
    return 0;
}
int creation(tarfile,path,v){
    return 0;
}
int listing(tarfile,v){
    return 0;
}
int extraction(tarfile,path){
    return 0;
}
