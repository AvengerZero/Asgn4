//header: include
#include <stdio.h>
#include <string.h>
#include <errno.h>	
#include <stdlib.h>	
#include <stdint.h> 

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <grp.h>

//#define _BSD_SOURCE
//static variable declaration
//NOTE: Everything deals with archive files as input, each archive file having the long header
int main(int argc, char *argv[]){
    if (argc<3){
        printf("mytar [ctxvS]f tarfile [ path [ ... ] ]");
        exit(1);
    }
    char* a=argv[1]; //for checking each letters
    int c=0,t=0,x=0,f=0,v=0,S=0,numpath=0;
    char* tarfile=0;
    char* path=0;
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
    if (c==1){(creation(tarfile,path,numpath,v));} //NOTE: NEW PARAMETERS FOR FUNCTION
    if (t==1){printf(listing());}
    if (x==1){printf(extraction());}
    return 0;
}
//note possible errors: fail to read file, fail to execute certain steps, including malloc
//Lacking in free command
//NOTE: IF PATH MORE THAN 256, ERROR
int creation(){ //leave it to Arjun
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
    //how to loop the int fd process to do all of this until numpath=0(what if it's already 0?): flag int done=0
    char buf[512]=0;
    int doneflag=0;
    //create new empty TA struct tapeArchive *tar=createnewTA();
    while (numpath!=0|done==0){
        
    int fd=open(tarfile,O_RDONLY);
    if (fd<0){ perror("open");return 1;}
    
        while (int a=read(fd,&buf,512)){
            if (a==-1){perror("read");return 1;}
            //put things inside [512] string into empty TArchive tar=fillTA(buf); //WRONG, NEED ARJUN
            if (verifyTA){return 1;}
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
                else{ printf("%s/n",tar->name);}
            }
            int c=b%512;
            if (c>0){ c=b/512+1;}
            else{ c=b/512;}
            read(fd,NULL,c*512); //skip the contents c worth of blocks
        }
        else {//if no path,print everything
        //checks to see if 
            if (v==1){printv(tar);}
            else{ printf("%s/n",tar->name);
            read(fd,NULL,b); //skip the contents x-steps WRONG
        }
    }
    }
    printf("%s,%d",tarfile,v);
    return 0;
}
int extraction(){ //implement a strlen to keep checking the file's remaining is <512

    printf("%s,%s",tarfile,path);
    return 0;
}
int tarVerify(TapeArchive *tar){
    if (/*check to see if checksum is correct-Arjun's, return 1 if false*/){
        perror("bad header"); 
        return 1;
    }
    if (S==1){
        for(int i=0;i<9;i++){ //magic number/version
            char u[8]="ustar/000";
            if (*(tar->magic+i)!=u[i]){     
                perror("bad header"); 
                return 1;
            }
        }
        
    else{
        if (strcmp(tar->magic,"ustar")){
            perror("bad header"); 
            return 1;
        }
    }
    return 0;
}
void printv(struct tapeArchive *tar){ //rule of thumb: copy data into your own for ease of manipulation
    //uses mode (left padded with 4 ASCII 0000, 4 ASCII numbers, 0(sticky)XXX (user-group.other)) and \0
    char persm[11] = { 0 };
    switch (tar->typeflag){
        case '5':
            perms[0]='d';
        case '2':
            perms[0]='l';
        default:
            perms[0]='-';
    }
    for (int i=0;i<3;i++){ //per char 
        int x =  tar->mode[4 + i] - 48; //atoi(octal[i]);
        if ((x-4)>=0){//read
            x -= 4;
            persm[(i * 3) + 1] = 'r';
        }
        else {
            persm[(i * 3) + 1]='-';
            
        }
        if ((x-2)>=0){//write
            x-=2;
            persm[(i * 3) + 2]='w';
        }
        else {
            persm[(i * 3) + 2]='-';
            
        }
        if ((x-1)>=0){//execute
            x-=1;
            persm[(i * 3) + 3]='x';
        }
        else {
            persm[(i * 3) + 3 ]='-';
        }
    }
    write(0, persm, sizeof(char) * );
    //translate tape->uid and gid into namespace (if failed, print numbers)
    struct passwd *pwd;
     unsigned int insertUser;
     if(tape->uidInt && S){
         perror("bad header");
     }
     (tape->uidInt) ? insertUser = tape->uidInt : insertUser = sizeTranslation(tape->uid);
    //int uid=tar->uid ? strtol into int?
    if ((pwd = getpwuid(insertUser)) != NULL)
        printf("%*s/",8, pwd->pw_name);
    else
        printf("%*d/",8, insertUser); 
     if(tape->gidInt && S){
         perror("bad header");
     }
    struct group *grp;
    (tape->gidInt) ? insertUser = tape->gidInt : insertUser = sizeTranslation(tape->gid);
    //int gid=tar->gid ??
    if ((grp = getgrgid(insertUser)) != NULL)
        printf("%*s ", 8,grp->gr_name);
    else
        printf("%*d ",8, gid);
    
    //translate time int into time format
    time_t time=strtol(tar->mtime,NULL,8);
    char tim[16];
    printf("%s "formatdate(tim,time));
    if(tar->prefix[0] != NULL){
        printf("%s/", tar->prefix);
    }
    printf("%s/n",tar->name); //consider if it's not NULL-terminated? (sectioned at 100?)
}

char* formatdate(char* str, time_t val)
{
        strftime(str, 16, "%Y-%m-%d %H:%M", localtime(&val));
        return str;
}
//timing of errors for header: listing, extracting, after getting header
        //todo: permission, verify (deals with strict),pseudo for extracting 
        //header->location, name, and stat stuff. then copy paste innards according to size, repeat