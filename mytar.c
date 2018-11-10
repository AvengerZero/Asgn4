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
static int v=0,S=0,numpath=0;
static char* tarfile=0;
static char* path=0;
//NOTE: Everything deals with archive files as input, each archive file having the long header
int main(int argc, char *argv[]){
    if (argc<3){
        printf("mytar [ctxvS]f tarfile [ path [ ... ] ]");
        exit(1);
    }
    char* a=argv[1]; //for checking each letters
    int c=0,t=0,x=0,f=0;
    while (;*a!='/0';a++){
        if (*a=='c'){c=1;}
        if (*a=='t'){t=1;}
        if (*a=='x'){x=1;}
        if (*a=='v'){v=1;}
        if (*a=='S'){S=1;}
    }
    if (*(--a)=='f'){f=1;}
    if (f==0||c+t+x!=1){
        printf("mytar [ctxvS]f tarfile [ path [ ... ] ]");
        exit(1);
    }
    tarfile=argv[2];
    if (argc>=4){path=argv[3]; numpath=argc-3;} //more than 1 argument (when to check search 256+?)
    if (c==1){printf(creation());} //NOTE: PLACEHOLDER
    if (t==1){printf(listing());}
    if (x==1){printf(extraction());}
    return 0;
}
//note possible errors: fail to read file, fail to execute certain steps, including malloc
//Lacking in free command
//NOTE: IF PATH MORE THAN 256, ERROR
int creation(){
    int fd=open(tarfile,O_CREAT|O_RDWR|O_TRUNC,770); //create new with perms
    if (fd<0){ perror("fd-open");return 1;}
    tapeArchive *tar=createArchiveFromFile(fd); 
    //remaining arguments taken as path
    //while loop if argv[x]!=NULL
    //set int fd pointer accordingly (recursion)
    //while opendir loop with int fd ^
    //if type isn't defined, printf error then go on
    //if type is directory, open everything inside it for tar
    //depth first, deepest directory
    //else, if v, printf file.
    //printArchiveToFile
    //note: make smaller function for recursion, book
    printf("%s,%s,%d",*tarfile,path,v);
    return 0;
}
int listing(){ //NOTE: CORRUPT HEADER=ABORT
    //INVALID CHECKSUM, MAGIC NUM, VERSION
    int fd=open(tarfile,O_RDONLY);
    if (fd<0){ perror("open");return 1;}
    char buf[512]=0;
    //read the thing, then skip size_t step towards next header.
    struct tapeArchive *tar=createnewTA();
    while (int a=read(fd,&buf,512)){
        if (a==-1){perror("read");return 1;}
        //put things inside [512] string into empty TArchive (handle uid?)
        tar=fillTA(buf);
        long int b=strtol(tar->size,NULL,8);
        if (path!=NULL){
            //compare path to name: use tar
            if (strlen(path)>256){
                perror("pathlen");return 1;
            }
            char *pfix=tar->prefix;
            char *nme=tar->name;
            strcat(pfix,nme); //final string in pfix
            char* yes=strstr(pfix,path); //find substring in string
            if (yes){ //do the traversal
                //print info
                if (v==1){printv(tar);}
                else{ printf("%s/n",tar->name);
            }
            else { //skip  
                read(fd,NULL,b); //skip the contents x-steps

            }
        }
        else {//if no path,print everything
        //checks to see if 
            if (v==1){printv(tar);}
            else{ printf("%s/n",tar->name);
            read(fd,NULL,b); //skip the contents x-steps
        }
    }
    printf("%s,%d",tarfile,v);
    return 0;
}
int extraction(){
    printf("%s,%s",tarfile,path);
    return 0;
}
int tarVerify(tarfile){
    
}
void printv(TapeArchive *tar){
    
    //uses mode (left padded with 4 ASCII 0000, 4 ASCII numbers, 0(sticky)XXX (user-group.other)) and \0
    //translate uid and gid into namespace
    //translate time int into time format
    //in printing, adds spaces (formatting)
}
//timing of errors for header: listing, extracting, after header
        //todo: permission, verify,pseudo for extracting 
        //header->location, name, and stat stuff. then copy paste innards according to size, repeat
