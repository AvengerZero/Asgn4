//header: include
#include <stdio.h>
#include <string.h>
#include <errno.h>	
#include <stdlib.h>	
#include <stdint.h> 

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

//#define _BSD_SOURCE
//static variable declaration
static int v=0,S=0;
static char* tarfile;
static char* path;
//NOTE: Everything deals with archive files as input, each archive file having the long header
int main(int argc, char *argv[]){
    if (argc!=3 ||argc!=4){
        printf("mytar [ctxvS]f tarfile [ path [ ... ] ]");
        exit(1);
    }
    char a=argv[1]; //for checking each letters
    int c=0,t=0,x=0,f=0;
    while (;a != '/0';a++){
        if (strcmp(&a,'c')==0){c=1;}
        if (strcmp(&a,'t')==0){t=1;}
        if (strcmp(&a,'x')==0){x=1;}
        if (strcmp(&a,'f')==0){f=1;}
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
    printf("%s,%s,%d",tarfile,path,v);
    return 0;
}
int listing(tarfile,v){
    //uses archive file, therefore, uses ARCHIVE HEADER
    //uses mode (left padded with 4 ASCII 0000, 4 ASCII numbers: 0(sticky)XXX (user-group.other)) and \0
    //uses file type for d or not
    //with file pointer (while loop), open and see if RIGHT directory (if command), if so, go further and repeat
    //else print name with dirent
    //if v is on, does the same, but now with lstat and dirent: permission (directory,user/group/other)
    //st_uid/st_gid (with getpwuid/getgrgid, st_size, st_mtime    
    printf("%s,%d",tarfile,v);
    return 0;
}
int extraction(tarfile,path){
    printf("%s,%s",tarfile,path);
    return 0;
}
