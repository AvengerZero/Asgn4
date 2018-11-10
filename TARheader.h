#ifndef TARheader
#define TARheader

typedef struct tapeArchive{
    char name[100];//
    char mode[8];//
    unsigned long long uidInt;//
    char uid[8];//
    unsigned long long gidInt;//
    char gid[8];//
    uint32_t maskSize;//
    unsigned long long sizeInt;//
    char size[12];//
    char mtime[12];//
    char chksum[8];
    char typeflag;//
    char linkname[100];
    char magic[6];//
    char version[2];//
    char uname[32];//
    char gname[32];//
    char devmajor[8];//
    char devminor[8];//
    char prefix[155];//
}tapeArchive;

int power(int base, int exponent);
void intToChar(int data, char *dest, int *chksum);
int endToStart(int start, int loop, mode_t idFilter, char *buffer, int *chksum);
void fillTapeArchive(int fd, char* path);
struct tapeArchive *createTapeArchive();
struct tapeArchive **createTapeArchiveList(int size);
struct tapeArchive *createArchiveFromFile(int fd);
void printArchiveToFile(int fd, struct tapeArchive *tape);
void writeBigEndian(int fd, unsigned long long num);
int endToStartLong(int start, int loop, unsigned long idFilter, char *buffer, int *chksum);
int readDirectoryDFS(int fd, const char *pathname);
void fillArrayBlank(char *array, int size);

#endif
