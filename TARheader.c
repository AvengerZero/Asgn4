#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sysmacros.h>
#include <grp.h>
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
	intToChar(data/8, dest --);
	dest = (data % 8) + 48;
    }
}

void fillTapeArchive(int fd, char* path)
{
    int i;
    struct stat *data = (struct stat *) malloc(sizeof(struct stat));
    stat(path, data);
    struct tapeArchive *tape = (struct tapeArchive *) malloc(sizeof(struct tapeArchive));
    /*fill typeflag*/
    if(S_ISDIR(data->st_mode)){
	/*Director*/
	tape->typeflag = 53;
    }else if(S_ISLNK(data->st_mode)){
	/*Symbolic Link*/
	tape->typeflag = 50;
    }else if(S_ISREG(data->st_mode)){
	/*Regular File*/
	tape->typeflag = 48;
    }

    /*user ID ? BIG-ENDIAN : char*/
    (data->st_uid >> 24) ?
	tape->uidInt = (MASK64 | data->st_uid) :
	endToStart(8, 8, data->st_uid, tape->uid);

    /*Group ID ? BIG-ENDIAN : char*/
    (data->st_gid >> 24) ? tape->gidInt = (MASK64 | data->st_gid) :
	endToStart(8, 8, data->st_gid, tape->gid);
    struct group *hold = getgrgid(data->st_gid);

    /*GName*/
    for(i = 0; i < sizeof(hold->gr_name); i++){
	tape->gname[i] = hold->gr_name[i];
	if(i == 31){
	    i = sizeof(hold->gr_name);
	}
    }
    /*Size*/

    if(data->st_size >> 36){
	tape->maskSize = MASK32;
	tape->sizeInt = data->st_size;
    }else{
	endToStart(12, 12, data->st_size, tape->size);
    }
    /*MTime*/
    //struct tm *update = localtime(data->st_mtim->tv_sec);

    /*UName*/

    /*Devmajor*/
    printf("%s\n", tape->devmajor);
    intToChar(major(data->st_dev), tape->devmajor + 8);
    printf("%s\n", tape->devmajor + 6);
    
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

void printArchiveToFile(int fd, struct tapeArchive *tape){
    write(fd, tape, 500);
    write(fd, "\0\0\0\0\0\0\0\0\0\0\0\0\0\\0\0\0\0\0\0\0\0\0\0\0", 12);
}

int main(int argv, char *argc[]){
    char *pathname = argc[1];
    fillTapeArchive(1, pathname);
    return 1;
}
