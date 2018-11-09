#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sysmacros.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <dirent.h>
#include "TARheader.h"
#define MASK64 0x8000000000000000
#define MASK32 0x80000000

int power(int base, int exponent){
    int i, ret = 1;
    for(i = 1; i <= exponent; i++){
	ret *= base;
    }
    return ret;
}

/*input a Character backwards*/
void intToChar(int data, char *dest){
    if(data){
	intToChar((data - (data % 8))/8, dest - 1);
	*dest = (data % 8) + 48;
    }
}

void fillTapeArchive(int fd, char* path, char *prevPath)
{
    int i, checkFind = 0;
    struct stat *data = (struct stat *) malloc(sizeof(struct stat));
    struct tapeArchive *tape = (struct tapeArchive *) malloc(sizeof(struct tapeArchive));
    stat(path, data);

    /*name and prefix*/
    if(strlen(path) < 100){
	for(i = 99 - strlen(path); i < 99; i++){
	    tape->name[i] = path[i - (99 - strlen(path))];
	    checkFind++;
	}
    }else{
	int slashSpot = 0;
	for(i = 98; i > 0; i--){
	    if(path[i] == '/'){
		slashSpot = i;
		i = 0;
	    }
	}
	if(slashSpot){
	    for(i = slashSpot; i > 0; i--){
		tape->name[99 - i] = path[slashSpot - i];
		checkFind++;
	    }

	    for(i = strlen(&(path[slashSpot + 1])); i >= 0;i--){
		tape->prefix[154 - (i)] =
		    path[strlen(path) - i];
		checkFind++;
	    }
	}else{
	    perror("NAME TOO LONG!");
	}
    }

    
    /*fill typeflag*/
    checkFind++;
    int mhold = data->st_mode;
    if(S_ISDIR(mhold)){
	/*Directory*/
	tape->typeflag = 53;
    }else if(S_ISLNK(mhold)){
	/*Symbolic Link*/
	tape->typeflag = 50;
    }else if(S_ISREG(mhold)){
	/*Regular File*/
	tape->typeflag = 48;
	/*Size*/
	if(data->st_size >> 33){
	    tape->maskSize = MASK32;
	    tape->sizeInt = data->st_size;
	}else{
	    endToStart(11, 11, data->st_size, tape->size);
	    checkFind += 11;
	}
    }

    /*File Permissions*/
    for(i = sizeof(tape->mode) - 2; i >= 0; i--){
	tape->mode[i] = (mhold & 0x7) + 48;
	mhold >>= 3;
	checkFind++;
    }

    /*user ID ? BIG-ENDIAN : char*/
    (data->st_uid >> 21) ? tape->uidInt = (MASK64 | data->st_uid) :
	endToStart(7, 7, data->st_uid, tape->uid);

    /*Group ID ? BIG-ENDIAN : char*/
    (data->st_gid >> 21) ? tape->gidInt = (MASK64 | data->st_gid) :
	endToStart(7, 7, data->st_gid, tape->gid);

    /*GName*/
    struct group *hold = getgrgid(data->st_gid);
    if(hold != NULL){
	for(i = sizeof(tape->gname) - strlen(hold->gr_name) - 1;
	    i < sizeof(tape->gname); i++){
	    tape->gname[i] = hold->
		gr_name[i- (sizeof(tape->gname)- strlen(hold->gr_name) - 1)];
	    checkFind++;
	}
    }
    
    /*MTime*/
    unsigned long timeMask = data->st_mtim.tv_sec;
    endToStartLong(11, 11, timeMask, tape->mtime);
    checkFind += 11;

    /*UName*/
    struct passwd *userName = getpwuid(data->st_uid);
    if(userName != NULL){
	for(i = sizeof(tape->uname) - strlen(userName->pw_name) - 1;
	    i < sizeof(tape->uname); i++){
	    tape->uname[i] = userName->
		pw_name[i- (sizeof(tape->uname)- strlen(userName->pw_name) - 1)];
	    checkFind++;
	}
    }

    /*Devmajor & Devminor*/
    if(major(data->st_dev)){
	intToChar(major(data->st_dev), tape->devmajor + 7);
    }else{
	*(tape->devmajor + 7) = 48;
    }
    if(minor(data->st_dev)){
	intToChar(minor(data->st_dev), tape->devminor + 7);
    }else{
	*(tape->devminor + 7) = 48;
    }

    printf("%i\n", checkFind);
    printArchiveToFile(fd, tape);
    
}

int endToStartLong(int start, int loop, unsigned long idFilter, char *buffer){
    int buff = idFilter & 0x7;
    if(buff || loop){
	*(buffer + start - endToStart(start, loop - 1, idFilter >> 3, buffer))
	  = buff + 48;
	return start - loop;
    }else{
	return start - loop;
    }
}

int endToStart(int start, int loop, unsigned int idFilter, char *buffer){
    int buff = idFilter & 0x7;
    if(buff || loop){
	*(buffer + start - endToStart(start, loop - 1, idFilter >> 3, buffer))
	  = buff + 48;
	return start - loop;
    }else{
	return start - loop;
    }
}

/*Create a Tape Archive Struct pointer*/
struct tapeArchive *createTapeArchive()
{
    struct tapeArchive tape = {
	.name = {0},
	.mode = {0},
	.uidInt = 0x0,
	.uid = {0},
	.gidInt = 0x0,
	.gid = {0},
	.maskSize = 0x0,
	.sizeInt = 0x0,
	.size = {0},
	.mtime = {0},
	.chksum = {0},
	.typeflag = 0,
	.linkname = {0},
	.magic = "ustar",
	.version[0] = 0,
	.version[1] = 0,
	.uname = {0},
	.gname = {0},
	.devmajor = {0},
	.devminor = {0},
	.prefix = {0}
    };
    return &tape;
}

/*Create a list of Tape Archive pointers of length [size]*/
struct tapeArchive **createTapeArchiveList(int size){
    return (struct tapeArchive **) malloc(sizeof(struct tapeArchive *) * size);
}

/*Take a File Directory and return a struct pointer 
  based on the data in that directory*/
struct tapeArchive *createArchiveFromFile(int fd){
    int i;
    char buffer;
    struct tapeArchive *tape = createTapeArchive();
    for(i = 0; i < 100; i++){
	read(fd, &buffer, sizeof(char));
	tape->name[i] = buffer;
    }
    for(i = 0; i < 8; i++){
	read(fd, &buffer, sizeof(char));
	tape->mode[i] = buffer;
    }
    for(i = 0; i < 8; i++){
	read(fd, &buffer, sizeof(char));
	tape->uid[i] = buffer;
    }
    for(i = 0; i < 8; i++){
	read(fd, &buffer, sizeof(char));
	tape->gid[i] = buffer;
    }
    for(i = 0; i < 12; i++){
	read(fd, &buffer, sizeof(char));
	tape->size[i] = buffer;
    }
    for(i = 0; i < 8; i++){
	read(fd, &buffer, sizeof(char));
	tape->mtime[i] = buffer;
    }
    for(i = 0; i < 8; i++){
	read(fd, &buffer, sizeof(char));
	tape->chksum[i] = buffer;
    }
    read(fd, &buffer, sizeof(char));
    tape->typeflag = buffer;
    for(i = 0; i < 100; i++){
	read(fd, &buffer, sizeof(char));
	tape->linkname[i] = buffer;
    }
    for(i = 0; i < 32; i++){
	read(fd, &buffer, sizeof(char));
	tape->uname[i] = buffer;
    }
    for(i = 0; i < 32; i++){
	read(fd, &buffer, sizeof(char));
	tape->gname[i] = buffer;
    }
    for(i = 0; i < 8; i++){
	read(fd, &buffer, sizeof(char));
	tape->devmajor[i] = buffer;
    }
    for(i = 0; i < 8; i++){
	read(fd, &buffer, sizeof(char));
	tape->devminor[i] = buffer;
    }
    for(i = 0; i < 155; i++){
	read(fd, &buffer, sizeof(char));
	tape->prefix[i] = buffer;
    }
    for(i = 0; i < 12; i++){
	read(fd, &buffer, sizeof(char));
	if(buffer){perror("Data corrupted!");};
    }
    return tape;
}

void writeBigEndian(int fd, unsigned long long num){
    if(num){
	writeBigEndian(fd, num >> 8);
	write(fd, &num, 1);
    }
}

void printArchiveToFile(int fd, struct tapeArchive *tape){
    int i;
    write(fd, tape->name, 100 * sizeof(char));
    write(fd, tape->mode, 8 * sizeof(char));
      /*if(tape->uidInt){
	for(i = 0; i < 8; i++){
	    write(fd, &(tape->uidInt), 1);
	    tape->uidInt >>= 8;
	}
    }else{
	write(fd, tape->uid, 8 * sizeof(char));
    }*/
    (tape->uidInt) ? writeBigEndian(fd, tape->uidInt):
	write(fd, tape->uid, 8 * sizeof(char));
    (tape->gidInt) ? writeBigEndian(fd, tape->gidInt):
	write(fd, tape->gid, 8 * sizeof(char));
    if(tape->maskSize){
	writeBigEndian(fd, tape->maskSize);
	writeBigEndian(fd, tape->sizeInt);
    }else{
	write(fd, tape->size, 12 * sizeof(char));
    }
    write(fd, tape->mtime, 12* sizeof(char));
    write(fd, tape->chksum, 8 * sizeof(char));
    write(fd, &(tape->typeflag), sizeof(char));
    write(fd, tape->linkname, 100 * sizeof(char));
    write(fd, tape->magic, 6 * sizeof(char));
    write(fd, tape->version, 2 * sizeof(char));
    write(fd, tape->uname, 32 * sizeof(char));
    write(fd, tape->gname, 32 * sizeof(char));
    write(fd, tape->devmajor, 8 * sizeof(char));
    write(fd, tape->devminor, 8 * sizeof(char));
    write(fd, tape->prefix, 155 * sizeof(char));
    write(fd, "\0\0\0\0\0\0\0\0\0\0\0\0", 12 * sizeof(char));
}

int readDirectoryDFS(int fd, DIR *dir){
    ;
}

int readDirectoryDFS(int fd, const char *pathname){
    DIR *dir;
    struct dirent *read;
    if(!(dir = opendir(pathname))){
	return 0;
    }
    if(!(read = readdir(dir))){
	return 0;
    }
    do{
	if(strcmp(read->d_name, ".") != 0 && strcmp(read->d_name, "..") != 0 &&
	   !(chdir(read->d_name))){
	}
    }while((read = readdir(dir)) != NULL);
    
    do{
	if (strcmp(read->d_name, ".") != 0 && strcmp(read->d_name, "..") != 0){
	    fillTapeArchive(fd, read->d_name, pathname);
	}
    }while((read = readdir(dir)) != NULL);
    return 1;
}

int main(int argv, char *argc[]){
    char *pathname = argc[1];
    readDirectoryDFS(open(argc[2], O_WRONLY | O_TRUNC | O_CREAT, 0644), pathname);
    return 1;
}
