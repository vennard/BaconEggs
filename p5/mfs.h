#ifndef __MFS_h__
#define __MFS_h__

#define MFS_DIRECTORY    (0)
#define MFS_REGULAR_FILE (1)

#define MFS_BLOCK_SIZE   (4096)

typedef struct __MFS_Stat_t {
    int type;   // MFS_DIRECTORY or MFS_REGULAR
    int size;   // bytes
    // note: no permissions, access times, etc.
} MFS_Stat_t;

typedef struct __MFS_DirEnt_t {
    char name[60];  // up to 60 bytes of name in directory (including \0)
    int  inum;      // inode number of entry (-1 means entry not used)
} MFS_DirEnt_t;


int MFS_Init(char *hostname, int port);
int MFS_Lookup(int pinum, char *name);
int MFS_Stat(int inum, MFS_Stat_t *m);
int MFS_Write(int inum, char *buffer, int block);
int MFS_Read(int inum, char *buffer, int block);
int MFS_Creat(int pinum, int type, char *name);
int MFS_Unlink(int pinum, char *name);
int MFS_Shutdown();
int verify(void);
int receive(void);
int transmit(void);
int sendpacket(void);

//server utility structs
typedef struct inode {
    int size;
    int type;
    int data_ptrs[14];
} inode;
extern inode inode_t;

typedef struct direntry {
    char name[60];
    int inum;
} direntry;
extern direntry direntry_t;

//server global vars
extern char rbuf[4096];

//server utility functions
void startfs(char* filesystem);
int writeblock(int loc, char *buf, int size);
char* readblock(int loc, int size);
int getentry(int ptr);
int getinode(int inum);
void seteol(int eol);
int geteol(void);
int creatdirentry(int ptr, char *name);
int nextinum(void);

#endif // __MFS_h__

