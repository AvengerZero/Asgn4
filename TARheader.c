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
	intToChar((data - (data % 8))/8, dest - 1);
	*dest = (data % 8) + 48;
    }
}

void fillTapeArchive(int fd, char* path)
{
    int i;
    
    /*name and prefix*/
    if(strlen(path) < 101){
	for(i = 100 - strlen(path); i < 100; i++){
	    tape->name[i] = path[i - (100 - strlen(path))];
	}
    }else{
	int slashSpot = 0;
	for(i = 99; i > 0; i--){
	    if(path[i] == '/'){
		slashSpot = i;
		i = 0;
	    }
	}
	if(slashSpot){
	    for(i = slashSpot; i > 0; i--){
		tape->name[100 - i] = path[slashSpot - i];
	    }
	    
	    for(i = 155; i > 0; i--){
		if(path[slashSpot + (155 - i) + 1]){
		    tape->prefix[155 - i] = path[slashSpot + (155 - i) + 1];
		}else{
		    i = 0;
		}
	    }
	}else{
	    perror("NAME TOO LONG!");
	}
    }

    
    struct stat *data = (struct stat *) malloc(sizeof(struct stat));
    struct tapeArchive *tape = (struct tapeArchive *) malloc(sizeof(struct tapeArchive));

    /*fill typeflag*/
    if(S_ISDIR(data->st_mode)){
	/*Directory*/
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
    printArchiveToFile(fd, tape);
    
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
    write(fd, tape->name, 100 * sizeof(char));
    write(fd, tape->mode, 8 * sizeof(char));
    (tape->uidInt) ? write(fd, tape->uidInt, sizeof(uint64_t)):
	write(fd, tape->uid, 8 * sizeof(char));
    (tape->gidInt) ? write(fd, tape->gidInt, sizeof(uint64_t)):
	write(fd, tape->gid, 8 * sizeof(char));
    if(tape->maskSize){
	write(fd, tape->maskSize, sizeof(uint32_t));
	write(fd, tape->sizeInt, sizeof(uint64_t));
    }else{
	write(fd, tape->size, 12 * sizeof(char));
    }
    write(fd, tape->mtime, 12* sizeof(char));
    write(fd, tape->chksum, 8 * sizeof(char));
    write(fd, tape->typeflag, sizeof(char));
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

int main(int argv, char *argc[]){
    char *pathname = argc[1];
    fillTapeArchive(open(argc[2], O_WRONLY | O_TRUNC | O_CREAT, 0644), pathname);
    return 1;
}
