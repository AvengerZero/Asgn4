#ifdef TARheader
#define

struct tapeArchive{
    char name[100];
    char mode[8];
    char uid[8];
    char grid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag[1];
    char linkname[100];
    char magic[6] = "ustar\0";
    char version[2] = "00";
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
}tapeArchive;

#endif
