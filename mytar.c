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

void printv(struct tapeArchive *tar, int S);

int main(int argc, char *argv[]){
    if (argc < 3){
        printf("mytar [ctxvS]f tarfile [ path [ ... ] ]");
        exit(1);
    }
    char *a = argv[1]; //for checking each letters
    int c = 0, t = 0, x = 0, f = 0,
	v = 0, S = 0, i = 0;
    for(; i < 3; i++){
	if(a[i] == 'c'){
	    c = 1;
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

    if(!(c + t + x)){
	printf("mytar [ctxvS]f tarfile [ path [ ... ] ]");
        exit(1);
    }
    
    if (c){
	 int fd = open(argv[2], O_RDWR | O_TRUNC | O_CREAT, 0644);
	 fillTapeArchive(fd, argv[3]);
	 readDirectoryDFS(fd, argv[3]);
	 char blank[1024];
	 fillArrayBlank(blank, 1024);
	 write(fd, blank, 1024 * sizeof(char));
	 close(fd);
    } //NOTE: PLACEHOLDER
    if (t){
	int fd = open(argv[2], O_RDONLY), checksize = 0;
	int twoBlanks = 0;
	int rounds = 0;
	struct tapeArchive *tape;
	char buffer[512];
	while(twoBlanks != 2){
	    tape = createArchiveFromFile(fd);
	    if(ifBlankBlock(tape)){
		twoBlanks = 0;
		checksize = sizeTranslation(tape->size);
		if(checksize % 512){
		    rounds = 1 + (checksize / 512);
		}else{
		    rounds = checksize / 512;
		}
		for(; rounds > 0; rounds--){
		    read(fd, buffer, sizeof(char) * 512);
		}
		printv(tape, S);
		
	    }else{
		twoBlanks++;
	    }
	}
	close(fd);
    }
    if (x){
	int fd = open(argv[2], O_RDONLY), checksize = 0,
	    twoBlanks = 0, rounds = 0;
	struct tapeArchive *tape;
	char pathname[255];
	char buffer[512];
	char saveDirectory[500];
	if(argv[3] != NULL){
	    getcwd(saveDirectory, sizeof(char) * 500);
	    chdir(argv[3]);
		/////////////////////////////if chdir fails
	}
	while(twoBlanks != 2){
	    tape = createArchiveFromFile(fd);
	    if(ifBlankBlock(tape)){
		twoBlanks = 0;
		checksize = sizeTranslation(tape->size);
		if(checksize % 512){
		    rounds = 1 + (checksize / 512);
		}else{
		    rounds = checksize / 512;
		}
		if(tape->prefix[0]){
		    strcat(pathname, tape->prefix);
		    strcat(pathname, "/");
		}
		strcat(pathname, tape->name);
		int writeFD;
		switch(tape->typeflag){
		    case '5':
			mkdir(pathname, sizeTranslation(tape->mode));
			
			break;
		    case '2':
			symlink(pathname, tape->linkname);
			break;
		    default:
			writeFD = open(pathname, O_WRONLY | O_TRUNC
				       | O_CREAT);
			for(; rounds > 0; rounds--){
			    read(fd, buffer, sizeof(char) * 512);
			    write(writeFD, buffer, sizeof(char) * 512);
			}
		}
		/*struct utimebuf *newTime = utimebuf *
		    malloc(sizeof(struct utimebuf));
		newTime->modtime = sizeTranslation(tape->mtime);
		utime(pathname, newTime);*/
	    }else{
		twoBlanks++;
	    }
	}
	close(fd);
	if(argv[3] != NULL){
	    char compareCWD[500];
	    getcwd(compareCWD, sizeof(char) * 500);
	    while(strcmp(compareCWD, saveDirectory)){
		char compareCWD[500];
		getcwd(compareCWD, sizeof(char) * 500);
		chdir("..");
	    }
	}
	return 0;
    }
    return 0;
}

void printv(struct tapeArchive *tar, int S){
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
	lengthName -= sizeof(pwd->pw_name);
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
	lengthName -= sizeof(grp->gr_name);
    }else{
	(tar->gidInt) ? (printf("%08llu", tar->gidInt)) :
	    (printf("%s", tar->gid));
	lengthName -= 8;
    }

    for(; lengthName > 0; lengthName--){
	printf(" ");
    }




    
    //translate time int into time format
//time_t time=strtol(tar->mtime,NULL,8);
// char tim[16];
// printf("%s "formatdate(tim,time));
// if(tar->prefix[0] != NULL){
//      printf("%s/", tar->prefix);
    printf("%s\n",tar->name);
    //consider if it's not NULL-terminated? (sectioned at 100?)
}
