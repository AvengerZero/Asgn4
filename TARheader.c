#include <stdio.h>
#include <TARheader.c>

tapeArchive *createTapeArchive();
{
    return (tapeArchive *) malloc(sizeof(tapeArchive));
}

tapeArchive **createTapeArchiveList(int size){
    return (tapeArchive **) malloc(sizeof(tapeArchive *) * size);
}

tapeArchive *createArchiveFromFile(int fd){
    int i;
    char buffer;
    tapeArchive *tape = createTapeArchive();
    for(i = 0; i < 100; i++){
	read(fd, &buffer, sizeof(char));
	tape->name[i] = buffer
    }
    for(i = 0; i < 8; i++){
	read(fd, &buffer, sizeof(char));
	tape->mode[i] = buffer
    }
    for(i = 0; i < 8; i++){
	read(fd, &buffer, sizeof(char));
	tape->uid[i] = buffer
    }
    for(i = 0; i < 8; i++){
	read(fd, &buffer, sizeof(char));
	tape->grid[i] = buffer
    }
    for(i = 0; i < 12; i++){
	read(fd, &buffer, sizeof(char));
	tape->size[i] = buffer
    }
    for(i = 0; i < 8; i++){
	read(fd, &buffer, sizeof(char));
	tape->mtime[i] = buffer
    }
    for(i = 0; i < 8; i++){
	read(fd, &buffer, sizeof(char));
	tape->chksum[i] = buffer
    }
    read(fd, &buffer, sizeof(char));
    tape->typeflag = buffer
    for(i = 0; i < 100; i++){
	read(fd, &buffer, sizeof(char));
	tape->linkname[i] = buffer
    }
    for(i = 0; i < 32; i++){
	read(fd, &buffer, sizeof(char));
	tape->uname[i] = buffer
    }
    for(i = 0; i < 32; i++){
	read(fd, &buffer, sizeof(char));
	tape->gname[i] = buffer
    }
    for(i = 0; i < 8; i++){
	read(fd, &buffer, sizeof(char));
	tape->devmajor[i] = buffer
    }
    for(i = 0; i < 8; i++){
	read(fd, &buffer, sizeof(char));
	tape->devminor[i] = buffer
    }
    for(i = 0; i < 155; i++){
	read(fd, &buffer, sizeof(char));
	tape->prefix[i] = buffer
    }
    for(i = 0; i < 12; i++){
	read(fd, &buffer, sizeof(char));
	if(buffer){perror("Data corrupted!");};
    }
    return tape;
}

void printArchiveToFile(int fd, tapeArchive *tape){
    write(fd, tape, 500);
}
