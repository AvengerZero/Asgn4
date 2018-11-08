#ifndef TARheader
#define TARheader

typedef struct tapeArchive{
    char name[100];
    char mode[8];
    uint64_t uidInt;//
    char uid[8];//
    uint64_t gidInt;//
    char gid[8];//
    uint32_t maskSize;//
    uint64_t sizeInt;//
    char size[12];//
    char mtime[12];
    char chksum[8];
    char typeflag;//
    char linkname[100];
    char magic[6];//
    char version[2];//
    char uname[32];
    char gname[32];//
    char devmajor[8];//
    char devminor[8];//
    char prefix[155];
}tapeArchive;

int power(int base, int exponent);
void intToChar(int data, char *dest);
int endToStart(int start, int loop, mode_t idFilter, char *buffer);
void fillTapeArchive(int fd, char* path);
struct tapeArchive *createTapeArchive();
struct tapeArchive **createTapeArchiveList(int size);
struct tapeArchive *createArchiveFromFile(int fd);
void printArchiveToFile(int fd, struct tapeArchive *tape);


#endif
