#include <stdio.h>
#include <string.h>
#include <errno.h>	
#include <stdlib.h>	
#include <stdint.h>
#include <sys/types.h>
#include <utime.h>
#include <fcntl.h>
#include <sys/sysmacros.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <dirent.h>
#include "TARheader.h"

//#define _BSD_SOURCE
//static variable declaration
//NOTE: Everything deals with archive files as input,
//each archive file having the long header

void printv(struct tapeArchive *tar, int S, int checksize);
void creation(int fd, char *data);
void listing(char *data, int S);
void extraction(char *tarfile, char *dest);
char* formatdate(char* str, time_t val);
void sizeback(int value, int times);

void sizeback(int value, int times){
    if(times){
	sizeback(value / 10, times - 1);
	if(value){
	    printf("%i", value % 10);
	}else{
	    printf(" ");
	}
    }
}

int main(int argc, char *argv[]){
    if (argc < 3){
        printf("mytar [ctxvS]f tarfile [ path [ ... ] ]");
        exit(1);
    }
    char *a = argv[1]; //for checking each letters
    int c = 0, t = 0, x = 0, f = 0,
	v = 0, S = 0, i = 0;
    int fd;
    for(; i < 3; i++){
	if(a[i] == 'c'){
	    c = 1;
	    fd = open(argv[2], O_RDWR | O_TRUNC | O_CREAT, 0644);
	}
	if(a[i] == 't'){
	    t = 1;
	}
	if(a[i] == 'x'){
	    x = 1;
	}
	if(a[i] == 'f'){
	    f = 1;
	}
	if(a[i] == 'v'){
	    v = 1;
	}
	if(a[i] == 'S'){
	    S = 1;
	}
    }

    if(c + t + x != 1){
	printf("mytar [ctxvS]f tarfile [ path [ ... ] ]");
        exit(1);
    }
    i = 0;
    if (t){
	listing(argv[2], S);
    }
    for(; i < argc - 3; i++){
	if(c){
	    creation(fd, argv[3 + i]);
	}
	
	if (x){
	    extraction(argv[2], argv[3 + i]);
	}
    }
    
    if(c){
	char blank[1024];
	fillArrayBlank(blank, 1024);
	write(fd, blank, 1024 * sizeof(char));
    }
    return 0;
}

void makeDirPath(char *path)
{
    char *slash = strrchr(path, '/' );
    if(slash != NULL) {
	*slash = 0;
	makeDirPath(path);
	*slash = '/';
    }
    mkdir(path,0777);
}


void extraction(char *tarfile, char *dest){
    int fd = open(tarfile, O_RDONLY), checksize = 0,
	twoBlanks = 0, rounds = 0;
    struct tapeArchive *tape;
    char pathname[255];
    char buffer[512];
    char saveDirectory[500];
    if(dest != NULL){
	getcwd(saveDirectory, sizeof(char) * 500);
	if(chdir(dest)){
	    makeDirPath(dest);
	    chdir(dest);
	}
    }
    while(twoBlanks != 2){
	tape = createArchiveFromFile(fd);
	if(ifBlankBlock(tape)){
	    twoBlanks = 0;
	    checksize = sizeTranslation(tape->size);
	    rounds = 1 + (checksize / 512);
	    if(tape->prefix[0]){
		strcat(pathname, tape->prefix);
		    strcat(pathname, "/");
	    }
	    strcat(pathname, tape->name);
	    int writeFD;
	    int total  = 0, i= 0;
	    printf("%s\n", pathname);
	    switch(tape->typeflag){
	    case '5':
		for(; i < strlen(tape->mode); i++){
		    total *= 8;
		    total += tape->mode[i] - 48;
		}
		makeDirPath(pathname);
		chmod(pathname, total);
		break;
	    case '2':
		symlink(pathname, tape->linkname);
		break;
	    default:
		writeFD = open(pathname, O_WRONLY | O_TRUNC
			       | O_CREAT, 644);
		for(; rounds > 0; rounds--){
		    int writeBlanc = read(fd, buffer, sizeof(char) * 512);
		    i = 0;
		    write(writeFD, buffer, sizeof(char) * writeBlanc);
		    for(i = 512 - writeBlanc; i > 0; i--){
			write(writeFD, " ", sizeof(char));
		    }
		}
		for(i = 0; i < strlen(tape->mode); i++){
		    total *= 8;
		    total += tape->mode[i] - 48;
		}
		makeDirPath(pathname);
		chmod(pathname, total);
	    }
	    fillArrayBlank(pathname, 255);
	    /*struct utimebuf *newTime = utimebuf *
	      malloc(sizeof(struct utimebuf));
	      newTime->modtime = sizeTranslation(tape->mtime);
	      utime(pathname, newTime);*/
	}else{
	    twoBlanks++;
	}
    }
    close(fd);
    if(dest != NULL){
	chdir(saveDirectory);
    }
}

void listing(char *data, int S){
    int fd = open(data, O_RDONLY);
    int checksize = 0;
    int twoBlanks = 0;
    int rounds = 0;
    int i;
    struct tapeArchive *tape;
    char buffer[512];
    while(twoBlanks != 2){
	tape = createArchiveFromFile(fd);
	if(ifBlankBlock(tape)){
		twoBlanks = 0;
		for(i = 0; i < 11; i++){
		    checksize *= 8;
		    if(tape->size[i] != '\0'){
			checksize += (tape->size[i] - 48);
		    }
		}
		if(checksize % 512){
		    rounds = 1 + (checksize / 512);
		}else{
		    rounds = checksize / 512;
		}
		printv(tape, S, checksize);
		for(; rounds > 0; rounds--){
		    read(fd, buffer, sizeof(char) * 512);
		}
		
	}else{
	    twoBlanks++;
	}
    }
    close(fd);
}

void creation(int fd, char *data){
    fillTapeArchive(fd, data);
    readDirectoryDFS(fd, data);
}

void printv(struct tapeArchive *tar, int S, int checksize){
    //rule of thumb: copy data into your own for ease of manipulation
    //uses mode (left padded with 4 ASCII 0000,
    //4 ASCII numbers, 0(sticky)XXX (user-group.other)) and \0
    char perms[11] = { 0 };
    switch (tar->typeflag){
        case '5':
	    perms[0]='d';
	    break;
        case '2':
	    perms[0]='l';
	    break;
        default:
            perms[0]='-';

    }
    
    for (int i=0;i<3;i++){ //per char 
        int x =  tar->mode[4 + i] - 48; //atoi(octal[i]);
        if ((x-4)>=0){//read
            x -= 4;
            perms[(i * 3) + 1] = 'r';
        }
        else {
            perms[(i * 3) + 1]='-';
            
        }
        if ((x-2)>=0){//write
            x-=2;
            perms[(i * 3) + 2]='w';
        }
        else {
            perms[(i * 3) + 2]='-';
            
        }
        if ((x-1)>=0){//execute
            x-=1;
            perms[(i * 3) + 3]='x';
        }
        else {
            perms[(i * 3) + 3 ]='-';
        }
    }
    write(0, perms, sizeof(char) * 10);
    write(0, " ", sizeof(char));


    int lengthName = 16;
    
    //translate tape->uid and gid into namespace (if failed, print numbers)
    struct passwd *pwd;
    unsigned long insertUser;
    if(tar->uidInt && S){
	perror("bad header");
    }
    (tar->uidInt) ? (insertUser = tar->uidInt) :
	(insertUser = sizeTranslation(tar->uid));
    //int uid=tar->uid ? strtol into int?
    if ((pwd = getpwuid(insertUser)) != NULL){
        printf("%s/", pwd->pw_name);
	lengthName -= strlen(pwd->pw_name);
    }else{
	printf("%08lu/", insertUser);
	lengthName -= 8;
    }
    if(tar->gidInt && S){
	perror("bad header");
    }
    
    struct group *grp;
    (tar->gidInt) ? (insertUser = tar->gidInt) :
	(insertUser = sizeTranslation(tar->gid));
    //int gid=tar->gid ??
    if ((grp = getgrgid(insertUser)) != NULL){
	printf("%s",grp->gr_name);
	lengthName -= strlen(grp->gr_name);
    }else{
	(tar->gidInt) ? (printf("%08llu", tar->gidInt)) :
	    (printf("%s", tar->gid));
	lengthName -= 8;
    }

    if(lengthName < 0){
	perror("names too long");
	exit(1);
    }

    for(; lengthName > 0; lengthName--){
	printf(" ");
    }

    printf(" ");

    if(checksize == 0){
	printf("       0");
    }else{
	sizeback(checksize, 8);
    }

    printf(" ");

    time_t time=strtol(tar->mtime,NULL,8);
    char tim[16];
    printf("%s ", formatdate(tim,time));

    if(tar->prefix[0] != '\0'){
	printf("%s/", tar->prefix);
    }
    printf("%s\n",tar->name);
    //consider if it's not NULL-terminated? (sectioned at 100?)
}

char* formatdate(char* str, time_t val)
{
    strftime(str, 36, "%Y-%m-%d %H:%M ", localtime(&val));
    return str;
}
