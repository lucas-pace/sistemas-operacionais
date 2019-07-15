/*
*  myfs.c - Implementacao de funcoes do sistema de arquivos
*
*  Autores: Lucas de Pace, André Caetano e Cristiano Nascimento
*  Projeto: Trabalho Pratico II - Sistemas Operacionais
*  Organizacao: Universidade Federal de Juiz de Fora
*  Departamento: Dep. Ciencia da Computacao
*
*
*/

#include <stdlib.h>
#include <string.h>
#include "myfs.h"
#include "disk.h"
#include "vfs.h"
#include "util.h"

/**
 O superbloco é essencialmente um metadado do sistema de arquivos e define o tipo, tamanho, status e informações do sistema de arquivos sobre
 outras estruturas de metadados (metadados de metadados). O superbloco é muito crítico para o sistema de arquivos e, portanto, é armazenado
 em várias cópias redundantes para cada sistema de arquivos. O superbloco é uma estrutura de metadados muito "de alto nível" para o
 sistema de arquivos.
 **/
#define SUPER_NUM_BLOCKS (3 * sizeof(unsigned int) + sizeof(char))
#define SUPER_BLOCKSIZE 0
#define SUPER_FREE_SPACE_SECTOR (sizeof(unsigned int) + sizeof(char))
#define SUPER_FIRST_BLOCK_SECTOR (2 * sizeof(unsigned int) + sizeof(char))

int myfsSlot = -1;

FSInfo const myfsInfo = {
        1,
        "myfs",
        myfsIsIdle,
        formatDisk,
        openFile,
        readFile,
        writeFile,
        closeFile,
        openDir,
        readDir,
        link,
        unlink,
        closeDir
};

struct file {
    Disk *disk;
    Inode *inode;
    unsigned int blockSize;
    unsigned int lastByteRead;
};

struct file {
    Disk *disk;
    Inode *inode;
    unsigned int blockSize;
    unsigned int lastByteRead;
};



File *openFiles[MAX_FDS] = {};

int installMyFS() {
    myfsSlot = vfsRegisterFS(&myfsInfo);
    return myfsSlot;
}

int myfsIsIdle(Disk *disk) {
    for (int i = 0; i < MAX_FDS; i++) {
        File *file = openFiles[i];

        if (file != NULL && diskGetId(disk) == diskGetId(file->disk))
            return 0;
    }

    return 1;
}

int formatDisk(Disk *disk, unsigned int blockSize) {
    unsigned char super[DISK_SECTORDATASIZE] = {0};
    unsigned char freeSpace[DISK_SECTORDATASIZE] = {0};
    ul2char(blockSize, &super[SUPER_BLOCKSIZE]);
    unsigned int numInodes = diskGetSize(disk) / blockSize / 8;

    for (int i = 1; i <= numInodes; i++) {
        Inode *inode = inodeCreate(i, disk);
        if (!inode)
            return -1;
        free(inode);
    }

    unsigned int freeSpaceSector = inodeAreaBeginSector() + numInodes / inodeNumInodesPerSector();
    unsigned int freeSpaceSize = (diskGetSize(disk) / blockSize) / (sizeof(unsigned char) * 8 * DISK_SECTORDATASIZE);
    ul2char(freeSpaceSector, &super[SUPER_FREE_SPACE_SECTOR]);
    unsigned int firstBlockSector = freeSpaceSector + freeSpaceSize;
    unsigned int numBlocks = (diskGetNumSectors(disk) - firstBlockSector) / (blockSize / DISK_SECTORDATASIZE);
    ul2char(firstBlockSector, &super[SUPER_FIRST_BLOCK_SECTOR]);
    ul2char(numBlocks, &super[SUPER_NUM_BLOCKS]);

    if (diskWriteSector(disk, 0, super) == -1)
        return -1;

    for (int i = 0; i < freeSpaceSize; i++) {
        if (diskWriteSector(disk, freeSpaceSector + i, freeSpace) == -1)
            return -1;

    }
    if (numBlocks > 0)
        return numBlocks;
    else return -1;
}

int openFile(Disk *disk, const char *path) {
    //TODO implementar openFile

    return -1;
}

int readFile(int fd, char *buf, unsigned int nbytes) {
    if (fd <= 0 || fd > MAX_FDS)
        return -1;
    File *file = openFiles[fd];
    if (!file) return -1;


    unsigned int bytesRead = 0;
    unsigned int fileSize = inodeGetFileSize(file->inode);
    unsigned int currentInode = file->currentByte / file->diskBlockSize;
    unsigned int offsetInode = file->currentByte % file->diskBlockSize;
    unsigned int currentBlock = inodeGetBlockAddr(file->inode, currentInode);
    unsigned char diskBuffer[DISK_SECTORDATASIZE];

    while (bytesRead + file->currentByte < fileSize && bytesRead < nbytes && currentBlock > 0) {
        unsigned int sectorsPerBlock = file->diskBlockSize / DISK_SECTORDATASIZE;
        unsigned int firstSector = offsetInode / DISK_SECTORDATASIZE;
        unsigned int firstByte = offsetInode % DISK_SECTORDATASIZE;

        for (int i = firstSector; i < sectorsPerBlock && bytesRead < nbytes; i++) {
            if (diskReadSector(file->disk, currentBlock + i, diskBuffer) == -1)
                return -1;

            for (int j = firstByte;
                 j < DISK_SECTORDATASIZE && bytesRead < nbytes && bytesRead + file->currentByte < fileSize; j++) {
                buf[bytesRead] = diskBuffer[j];
                bytesRead++;
            }
            firstByte = 0;
        }

        offsetInode = 0;
        currentInode++;
        currentBlock = inodeGetBlockAddr(file->inode, currentInode);
    }

    file->currentByte += bytesRead;
    return bytesRead;
}

int writeFile(int fd, const char *buf, unsigned int nbytes) {
    //TODO implementar writeFile

    return -1;
}

int closeFile(int fd) {
    if (fd <= 0 || fd > MAX_FDS)
        return -1;

    File *file = openFiles[fd];
    if (!file)
        return -1;

    openFiles[fd - 1] = NULL;
    free(file->inode);
    free(file);

    return 0;
}

int openDir(Disk *disk, const char *path) {
    //TODO implementar openDir

    return -1;
}

int readDir(int fd, char *filename, unsigned int *inumber) {

    if (fd <= 0 || fd > MAX_FDS)
        return -1;
    File *file = openFiles[fd];

    if (!file || inodeGetFileType(file->inode) != FILETYPE_DIR)
        return -1;

    Directory directory;
    int bytesRead = myfsRead(fd, (char *) &directory, sizeof(Directory));

    if (bytesRead == -1)
        return -1;
    if (bytesRead < sizeof(Directory))
        return 0;

    strcpy(filename, directory.filename);
    *inumber = directory.inumber;

    return 1;
}

int link(int fd, const char *filename, unsigned int inumber) {
    if (fd <= 0 || fd > MAX_FDS)
        return -1;
    File *file = openFiles[fd];

    if (!file || inodeGetFileType(file->inode) != FILETYPE_DIR)
        return -1;
    
    Inode* inodeLink = inodeLoad(inumber, dir->disk);
    if(!inodeLink)
        return -1;

    Directory directory;
    strcpy(directory.filename, filename);
    directory.inumber = inumber;
    unsigned int previousCurrentByte = dir->currentByte;
    unsigned int previousDirSize = inodeGetFileSize(dir->inode);
    dir->currentByte = previousDirSize;
    int bytesWritten = myfsWrite(fd, (const char*) &directory, sizeof(Directory));
    dir->currentByte = previousCurrentByte;

    if(bytesWritten != sizeof(Directory))
    {
        inodeSetFileSize(dir->inode, previousDirSize);
        inodeSave(dir->inode);
        free(inodeLink);
        return -1;
    }

    inodeSetRefCount(inodeLink, inodeGetRefCount(inodeLink) + 1);
    inodeSave(inodeLink);
    free(inodeLink);
    return 0;
}

int unlink(int fd, const char *filename)
    if (fd <= 0 || fd > MAX_FDS)
        return -1;
    File *file = openFiles[fd];

    if (!file || inodeGetFileType(file->inode) != FILETYPE_DIR)
        return -1;

    unsigned int previousCurrentByte = dir->currentByte;
    dir->currentByte = 0;
    Directory directory;
    unsigned int inumber = 0;
    while(myfsRead(fd, (char*) &directory, sizeof(Directory)) == sizeof(Directory))
    {
        if(strcmp(directory.filename, filename) == 0)
        {
            inumber = directory.inumber;
            unsigned int currentByte = dir->currentByte;
            unsigned int nextByte = dir->currentByte + sizeof(Directory);
            dir->currentByte = nextByte;
            while(myfsRead(fd, (char*) &directory, sizeof(Directory)) == sizeof(Directory))
            {
                dir->currentByte = currentByte;
                myfsWrite(fd, (char*) &directory, sizeof(Directory));
                currentByte += sizeof(Directory);
                nextByte += sizeof(Directory);
                dir->currentByte = nextByte;
            }

            unsigned int previousDirSize = inodeGetFileSize(dir->inode);
            inodeSetFileSize(dir->inode, previousDirSize - sizeof(Directory));
            break;
        }
    }

    dir->currentByte = previousCurrentByte;
    if(inumber == 0) 
        return -1;

    Inode* inodeUnlink = inodeLoad(inumber, dir->disk);
    if(inodeUnlink == NULL)
        return -1;

    unsigned int previousRefCount = inodeGetRefCount(inodeUnlink);
    inodeSetRefCount(inodeUnlink, previousRefCount - 1);

    if(previousRefCount == 1)
    {
        unsigned int blockCount = 0;
        unsigned int currentBlock = inodeGetBlockAddr(inodeUnlink, blockCount);

        while (currentBlock > 0)
        {
            __setBlockFree(dir->disk, currentBlock);
            blockCount++;
            currentBlock = inodeGetBlockAddr(inodeUnlink, blockCount);
        }

        inodeClear(inodeUnlink);
    }
    inodeSave(inodeUnlink);
    free(inodeUnlink);
    return 0;
}

int closeDir(int fd) {
    //TODO implementar closeDir

    return -1;
}