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
    int numBlocks = -1;

    //TODO implementar formatDisk

    return numBlocks;
}

int openFile (Disk *disk, const char *path) {
    int fd = -1;

    File *file = malloc(sizeof(File));
    file->disk = disk;

    //TODO implementar openFile

    return fd;
}

int readFile (int fd, char *buf, unsigned int nbytes) {
    int numBytesRead = 0;

    //TODO implementar redFile

    return numBytesRead;
}

int writeFile (int fd, const char *buf, unsigned int nbytes) {
    //TODO implementar writeFile

    return -1;
}

int closeFile (int fd) {
    File *file = openFiles[fd];
    if (file == NULL)
        return -1;

    openFiles[fd] = NULL;
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
