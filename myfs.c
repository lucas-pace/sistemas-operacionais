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

File *openFiles[MAX_FDS] = {};

int installMyFS () {
    myfsSlot = vfsRegisterFS(&myfsInfo);
    return myfsSlot;
}

int myfsIsIdle (Disk *disk) {
    for (int i = 0; i < MAX_FDS; i++) {
        File *file = openFiles[i];

        if (file != NULL && diskGetId(disk) == diskGetId(file->disk))
            return 0;
    }

    return 1;
}

int formatDisk (Disk *disk, unsigned int blockSize) {
    //TODO implementar formatDisk
    unsigned char super[DISK_SECTORDATASIZE] = {0};
    unsigned char freeSpace[DISK_SECTORDATASIZE] = {0};

    ul2char(blockSize, &super[SUPER_BLOCKSIZE]);

    unsigned int numInodes = (diskGetSize(disk) / blockSize) / 8;

    for(int i=1; i <= numInodes; i++) {

        Inode* inode = inodeCreate(i, disk);
        if(!inode)
            return -1;
        free(inode);

    }

    unsigned int freeSpaceSector = inodeAreaBeginSector() + numInodes / inodeNumInodesPerSector();
    unsigned int freeSpaceSize   = (diskGetSize(disk) / blockSize) / (sizeof(unsigned char) * 8 * DISK_SECTORDATASIZE);

    ul2char(freeSpaceSector, &super[SUPER_FREE_SPACE_SECTOR]);

    unsigned int firstBlockSector = freeSpaceSector + freeSpaceSize;
    unsigned int numBlocks        = (diskGetNumSectors(disk) - firstBlockSector) / (blockSize / DISK_SECTORDATASIZE);

    ul2char(firstBlockSector, &super[SUPER_FIRST_BLOCK_SECTOR]);
    ul2char(numBlocks, &super[SUPER_NUM_BLOCKS]);

    if(diskWriteSector(disk, 0, super) == -1 )
        return -1;

    for(int i=0; i < freeSpaceSize; i++) {

        if(diskWriteSector(disk, freeSpaceSector + i, freeSpace) == -1)
            return -1;

    }

    return numBlocks > 0 ? numBlocks : -1;
}

int openFile (Disk *disk, const char *path) {
    //TODO implementar openFile

    return -1;
}

int readFile (int fd, char *buf, unsigned int nbytes) {
    //TODO implementar readFile

    return -1;
}

int writeFile (int fd, const char *buf, unsigned int nbytes) {
    //TODO implementar writeFile

    return -1;
}

int closeFile (int fd) {
    if(fd <= 0 || fd > MAX_FDS)
        return -1;

    File *file = openFiles[fd];
    if (!file)
        return -1;

    openFiles[fd-1] = NULL;
    free(file->inode);
    free(file);

    return 0;
}

int openDir (Disk *disk, const char *path) {
    //TODO implementar openDir

    return -1;
}

int readDir (int fd, char *filename, unsigned int *inumber) {
    //TODO implementar readDir

    return -1;
}

int link (int fd, const char *filename, unsigned int inumber) {
    //TODO implementar link

    return -1;
}

int unlink (int fd, const char *filename) {
    //TODO implementar unlink

    return -1;
}

int closeDir (int fd) {
    //TODO implementar closeDir

    return -1;
}