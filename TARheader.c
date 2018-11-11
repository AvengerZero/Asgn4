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
void intToChar(int data, char *dest, int *chksum){
    if(data){
	intToChar((data - (data % 8))/8, dest - 1, chksum);
	*dest = (data % 8) + 48;
	*chksum += (data % 8) + 48;
    }
}

void fillTapeArchive(int fd, char* path)
{
    int i, checkFind = 8*32 + 'u' + 's' + 't' + 'a' + 'r' + '0' + '0';
    struct stat *data = (struct stat *) malloc(sizeof(struct stat));
    struct tapeArchive *tape = createTapeArchive();
    stat(path, data);

    /*name and prefix*/
    if(strlen(path) < 101){
	for(i = 0; i < 99; i++){
	    tape->name[i] = path[i];
	    checkFind += path[i];
	    if(path[i] == '\0'){
		i = 100;
	    }
	}
	if(strlen(path) == 100){
	    tape->name[99] = path[99];
	    checkFind += path[99];
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
	    for(i = 0; i < slashSpot; i++){
		tape->name[99 - i] = path[slashSpot - i];
		checkFind += path[i];
	    }

	    for(i = slashSpot + 1; i < 155; i++){
		tape->prefix[i - slashSpot - 1] = path[i];
		checkFind += path[i];
		if(path[i] == '\0'){
		    i = 155;
		}
	    }
	}else{
	    perror("NAME TOO LONG!");
	    exit(1);
	}
    }

    
    /*fill typeflag*/
    int mhold = data->st_mode;
    if(S_ISDIR(mhold)){
	/*Directory*/
	tape->typeflag = 53;
	checkFind += 53;
	for(i = 99; tape->name[i] == '\0'; i--);
	if(i != 99){
	    tape->name[i+1] = '/';
	    checkFind += '/';
	}else{
	    perror("NAME TOO LONG");
	    exit(1);
	}
    }else if(S_ISLNK(mhold)){
	/*Symbolic Link*/
	tape->typeflag = 50;
	checkFind += 50;
	readlink(path, tape->linkname, 100 * sizeof(char));
    }else if(S_ISREG(mhold)){
	/*Regular File*/
	tape->typeflag = 48;
	checkFind += 48;
	/*Size*/
	if(data->st_size >> 33){
	    tape->maskSize = MASK32;
	    tape->sizeInt = data->st_size;
	}else{
	    endToStart(11, 11, data->st_size, tape->size, &checkFind);
	}
    }

    /*File Permissions*/
    for(i = sizeof(tape->mode) - 2; i >= 3; i--){
	tape->mode[i] = (mhold & 0x7) + 48;
	checkFind += (mhold & 0x7) + 48;
	mhold >>= 3;
    }
    for(; i >= 0; i--){
	tape->mode[i] = (mhold & 0x0) + 48;
	checkFind += (mhold & 0x0) + 48;
	mhold >>= 3;
    }

    /*user ID ? BIG-ENDIAN : char*/
    (data->st_uid >> 21) ? tape->uidInt = (MASK64 | data->st_uid) :
	endToStart(7, 7, data->st_uid, tape->uid, &checkFind);

    /*Group ID ? BIG-ENDIAN : char*/
    (data->st_gid >> 21) ? tape->gidInt = (MASK64 | data->st_gid) :
		   endToStart(7, 7, data->st_gid, tape->gid, &checkFind);

    /*GName*/
    struct group *hold = getgrgid(data->st_gid);
    if(hold != NULL){
	for(i = 0; i < strlen(hold->gr_name); i++){
	    tape->gname[i] = hold->gr_name[i];
	    checkFind += hold->gr_name[i];
	}
    }
    
    /*MTime*/
    unsigned long timeMask = data->st_mtim.tv_sec;
    endToStartLong(11, 11, timeMask, tape->mtime, &checkFind);

    /*UName*/
    struct passwd *userName = getpwuid(data->st_uid);
    if(userName != NULL){
	for(i = 0; i < strlen(userName->pw_name); i++){
	    tape->uname[i] = userName->pw_name[i];
	    checkFind += userName->pw_name[i];
	}
    }

    /*Devmajor & Devminor*/
    if(major(data->st_dev)){
	intToChar(major(data->st_dev), tape->devmajor + 7, &checkFind);
    }else{
	*(tape->devmajor + 7) = 48;
    }
    if(minor(data->st_dev)){
	intToChar(minor(data->st_dev), tape->devminor + 7, &checkFind);
    }else{
	*(tape->devminor + 7) = 48;
    }

    if(tape->uidInt){
	chkLong(tape->uidInt, &checkFind);
    }
    if(tape->gidInt){
	chkLong(tape->gidInt, &checkFind);
    }
    if(tape->maskSize){
	chkLong1(tape->maskSize, &checkFind);
	chkLong(tape->sizeInt, &checkFind);
    }
    
    intToChar(checkFind, tape->chksum + 6, &i);
    char *ptrFollow = tape->chksum + 6;
    for(; ptrFollow >= tape->chksum; ptrFollow--){
	if(!(*ptrFollow)){
	    *ptrFollow = 48;
	}
    }
    printArchiveToFile(fd, tape);
    if(tape->typeflag == 48){
	printFileToOut(fd, path);
    }
    free(tape);
    
}

void chkLong1(uint32_t add, int *chksum){
    int i = 0;
    for(; i < 8; i++){
	*chksum += add & 0xff;
	add >>= 3;
    }
}

void chkLong(unsigned long long add, int *chksum){
    int i = 0;
    for(; i < 8; i++){
	*chksum += add & 0xff;
	add >>= 8;
    }
}

void printFileToOut(int fd, char *pathname){
    int openPath = open(pathname, O_RDONLY), i = 0;
    unsigned char buffer;
    while(read(openPath, &buffer, sizeof(char))){
	i++;
	write(fd, &buffer, sizeof(char));
    }
    buffer = 0;
    while(i % 512){
	write(fd, &buffer, sizeof(char));
	i++;
    }
}

int endToStartLong(int start, int loop,
		   unsigned long idFilter, char *buffer, int *chksum){
    int buff = idFilter & 0x7;
    if(buff || loop){
	*(buffer + start -
	  endToStartLong(start, loop - 1, idFilter >> 3, buffer, chksum))
	  = buff + 48;
	*chksum += buff + 48;
	return start - loop;
    }else{
	return start - loop;
    }
}

int endToStart(int start, int loop,
	       unsigned int idFilter, char *buffer, int *chksum){
    int buff = idFilter & 0x7;
    if(buff || loop){
	*(buffer + start -
	  endToStart(start, loop - 1, idFilter >> 3, buffer, chksum))
	  = buff + 48;
	*chksum += buff + 48;
	return start - loop;
    }else{
	return start - loop;
    }
}

void fillArrayBlank(char *array, int size){
    int i = 0;
    for(; i < size; i++){ array[i] = '\0'; }
}
/*Create a Tape Archive Struct pointer*/
struct tapeArchive *createTapeArchive()
{
    struct tapeArchive *tape =
      (struct tapeArchive *)malloc (sizeof(struct tapeArchive));
    fillArrayBlank(tape->name, 100);
    fillArrayBlank(tape->mode, 8);
    tape->uidInt = 0x0;
    fillArrayBlank(tape->uid, 8);
    tape->gidInt = 0x0;
    fillArrayBlank(tape->gid, 8);
    tape->maskSize = 0x0;
    tape->sizeInt = 0x0;
    fillArrayBlank(tape->size, 12);
    tape->size[10] = 48;
    fillArrayBlank(tape->mtime, 12);
    fillArrayBlank(tape->chksum, 8);
    fillArrayBlank(tape->linkname, 100);
    tape->typeflag = 0;
    fillArrayBlank(tape->uname, 32);
    fillArrayBlank(tape->gname, 32);
    fillArrayBlank(tape->devmajor, 8);
    fillArrayBlank(tape->devminor, 8);
    fillArrayBlank(tape->prefix, 155);
    tape->magic[0] = 'u';
    tape->magic[1] = 's';
    tape->magic[2] = 't';
    tape->magic[3] = 'a';
    tape->magic[4] = 'r';
    tape->magic[5] = 0;
    tape->version[0] = 48;
    tape->version[1] = 48;
    tape->version[2] = '\0';
    return tape;
}

/*Create a list of Tape Archive pointers of length [size]*/
struct tapeArchive **createTapeArchiveList(int size){
    return (struct tapeArchive **)
      malloc(sizeof(struct tapeArchive *) * size);
}

/*Take a File Directory and return a struct pointer 
  based on the data in that directory*/
struct tapeArchive *createArchiveFromFile(int fd){
    int i;
    char buffer;
    unsigned long long hold;
    struct tapeArchive *tape = createTapeArchive();
    for(i = 0; i < 100; i++){
	read(fd, &buffer, sizeof(char));
	tape->name[i] = buffer;
    }
    for(i = 0; i < 8; i++){
	read(fd, &buffer, sizeof(char));
	tape->mode[i] = buffer;
    }
    
    read(fd, &buffer, sizeof(char));
    if(buffer & 0x80){
	hold = buffer-0x80;
	for(i = 1; i < 8; i++){
	    hold *= 256;
	    read(fd, &buffer, sizeof(char));
	    hold += buffer;
	}
	tape->uidInt= hold;
    }else{
	tape->uid[0] = buffer;
	for(i = 1; i < 8; i++){
	    read(fd, &buffer, sizeof(char));
	    tape->uid[i] = buffer;
	}
    }
    
    
    read(fd, &buffer, sizeof(char));
    if(buffer & 0x80){
	hold = buffer-0x80;
	for(i = 1; i < 8; i++){
	    hold *= 256;
	    read(fd, &buffer, sizeof(char));
	    hold += buffer;
	}
	tape->gidInt = hold;
    }else{
	tape->gid[0] = buffer;
	for(i = 1; i < 8; i++){
	    read(fd, &buffer, sizeof(char));
	    tape->gid[i] = buffer;
	}
    }
    
    read(fd, &buffer, sizeof(char));
    if(buffer & 0x80){
	uint32_t need = buffer-0x80;
	for(i = 1; i < 4; i++){
	    need *= 256;
	    read(fd, &buffer, sizeof(char));
	    need += buffer;
	}
	tape->maskSize = need;
	read(fd, &buffer, sizeof(char));
	hold = buffer-0x80;
	for(i = 1; i < 8; i++){
	    hold *= 256;
	    read(fd, &buffer, sizeof(char));
	    hold += buffer;
	}
	tape->sizeInt = hold;
    }else{
	tape->size[0] = buffer;
	for(i = 1; i < 12; i++){
	    read(fd, &buffer, sizeof(char));
	    tape->size[i] = buffer;
	}
    }
    for(i = 0; i < 12; i++){
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
    
    for(i = 0; i < 6; i++){
	read(fd, &buffer, sizeof(char));
	tape->magic[i] = buffer;
    }
    /*
    if(strcmp(tape->magic, "ustar")){
	printf("File Corrupted");
    }*/
    for(i = 0; i < 2; i++){
	read(fd, &buffer, sizeof(char));
	tape->version[i] = buffer;
    }
    /*
    if(strcmp(tape->version, "00")){
	printf("File Corrupted");
	}*/
    
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
    char *magic = (char *) malloc(sizeof(char));
    *magic = 'u';
    write(fd, magic, sizeof(char));
    *magic = 's';
    write(fd, magic, sizeof(char));
    *magic = 't';
    write(fd, magic, sizeof(char));
    *magic = 'a';
    write(fd, magic, sizeof(char));
    *magic = 'r';
    write(fd, magic, sizeof(char));
    *magic = '\0';
    write(fd, magic, sizeof(char));
    *magic = '0';
    write(fd, magic, sizeof(char));
    write(fd, magic, sizeof(char));
    write(fd, tape->uname, 32 * sizeof(char));
    write(fd, tape->gname, 32 * sizeof(char));
    write(fd, tape->devmajor, 8 * sizeof(char));
    write(fd, tape->devminor, 8 * sizeof(char));
    write(fd, tape->prefix, 155 * sizeof(char));
    write(fd, "\0\0\0\0\0\0\0\0\0\0\0\0", 12 * sizeof(char));
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
    char pathBuffer[255];
    do{
	if (strcmp(read->d_name, ".") != 0 &&
	    strcmp(read->d_name, "..") != 0){
	    memset(pathBuffer, '\0', sizeof(pathBuffer));
	    strcat(pathBuffer, pathname);
	    strcat(pathBuffer, "/");
	    strcat(pathBuffer, read->d_name);
	    if(opendir(pathname) != NULL){
		fillTapeArchive(fd, pathBuffer);
		readDirectoryDFS(fd, pathBuffer);
	    }
	}
    }while((read = readdir(dir)) != NULL);

    rewinddir(dir);

    while((read = readdir(dir)) != NULL){
	if (strcmp(read->d_name, ".") != 0 &&
	    strcmp(read->d_name, "..") != 0){
	    memset(pathBuffer, '\0', sizeof(pathBuffer));
	    strcat(pathBuffer, pathname);
	    strcat(pathBuffer, "/");
	    strcat(pathBuffer, read->d_name);
	    if(opendir(pathname) == NULL){
		fillTapeArchive(fd, pathBuffer);
	    }
	}
    }
    return 1;
}

void printTapeArchive(struct tapeArchive *tape){
    printf("name: %s\n", tape->name);
    printf("mode: %s\n", tape->mode);
    (tape->uidInt) ? printf("uid: %llu\n", tape->uidInt) :
	printf("uid: %s\n", tape->uid);
    (tape->gidInt) ? printf("gid: %llu\n", tape->gidInt) :
	printf("gid: %s\n", tape->gid);
    (tape->maskSize) ? printf("size: %u%llu\n",
			      tape->maskSize, tape->sizeInt) :
      printf("size: %s\n", tape->size);
    printf("mtime: %s\n", tape->mtime);
    printf("chksum: %s\n", tape->chksum);
    printf("typeflag: %c\n", tape->typeflag);
    printf("linkname: %s\n", tape->linkname);
    printf("magic: %s\n", tape->magic);
    printf("version: %s\n", tape->version);
    printf("uname: %s\n", tape->uname);
    printf("gname: %s\n", tape->gname);
    printf("devmajor: %s\n", tape->devmajor);
    printf("devminor: %s\n", tape->devminor);
    printf("prefix: %s\n", tape->prefix);
}

int ifStringBlank(char *check, int size){
    int i;
    for(i = 0; i < size; i++){
	if(check[i]){
	    return 1;
	}
    }
    return 0;
}

int sizeTranslation(char *intString){
  int total = 0, i = 0;
  for(; i < sizeof(intString); i++){
	total *= 8;
	total += (*intString - 48);
    }
    return total;
}

/*takes a tape archive and check if it is blank, return 0 if not*/
int ifBlankBlock(tapeArchive *tape){
    if(tape->uidInt || tape->gidInt || tape->maskSize ||
       ifStringBlank(tape->name, 100) ||
       ifStringBlank(tape->mode, 8) ||
       ifStringBlank(tape->uid, 8) ||
       ifStringBlank(tape->gid, 8) ||
       ifStringBlank(tape->size, 12) ||
       ifStringBlank(tape->mtime, 12) ||
       ifStringBlank(tape->chksum, 8) ||
       tape->typeflag ||
       ifStringBlank(tape->linkname, 100) ||
       ifStringBlank(tape->magic, 6) ||
       ifStringBlank(tape->version, 2) ||
       ifStringBlank(tape->uname, 32) ||
       ifStringBlank(tape->gname, 32) ||
       ifStringBlank(tape->devmajor, 8) ||
       ifStringBlank(tape->devminor, 8) ||
       ifStringBlank(tape->prefix, 155)){
	return 1;
    }
    return 0;
}


/*int main(int argv, char *argc[]){
    char *pathname = argc[1];
    int fd = open(argc[2], O_RDWR | O_TRUNC | O_CREAT, 0644);
    fillTapeArchive(fd, pathname);
    readDirectoryDFS(fd, pathname);
    char blank[1024];
    fillArrayBlank(blank, 1024);
    write(fd, blank, 1024 * sizeof(char));
    close(fd);

    Extract
    int fd = open(argc[1], O_RDONLY);
    struct tapeArchive *tape = createArchiveFromFile(fd);
    printTapeArchive(tape);
    close(fd);
    return 1;
}*/
