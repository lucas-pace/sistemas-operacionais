/*
*  myfs.c - Implementacao de funcoes do sistema de arquivos
*
*  Autores: André Caetano, Cristiano Nascimento e Lucas de Pace
*  Projeto: Trabalho Pratico II - Sistemas Operacionais
*  Organizacao: Universidade Federal de Juiz de Fora
*  Departamento: Dep. Ciencia da Computacao
*
*
*/

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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

FSInfo myfsInfo = {
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

struct directory {
    char path[MAX_FILENAME_LENGTH + 1];
    int numInode;
};

File *openFiles[MAX_FDS] = {NULL};

//{ Inicio - Metodos auxiliares
int firstZeroBit(unsigned char byte) {
    unsigned char mask = 1;

    for (int i = 0; i < sizeof(unsigned char); i++) {
        if( (mask & byte) == 0 )
            return i;

        mask <<= (unsigned char) 1;
    }

    return -1;
}

unsigned char setBitToOne(unsigned char byte, unsigned int bit) {
    unsigned char mask = (unsigned char) 1 << bit;
    return byte | mask;
}

unsigned char setBitToZero(unsigned char byte, unsigned int bit) {
    unsigned char mask = ((unsigned char) 1 << bit);
    mask = ~mask;
    return byte & mask;
}

unsigned int findFreeBlock(Disk *disk) {
    unsigned char buffer[DISK_SECTORDATASIZE];
    if (diskReadSector(disk, 0, buffer) == -1)
        return -1;

    unsigned int sectorsPerBlock;
    char2ul(&buffer[SUPER_BLOCKSIZE], &sectorsPerBlock);
    sectorsPerBlock /= DISK_SECTORDATASIZE;

    unsigned int numBlocks;
    char2ul(&buffer[SUPER_NUM_BLOCKS], &numBlocks);

    unsigned int firstBlock;
    char2ul(&buffer[SUPER_FIRST_BLOCK_SECTOR], &firstBlock);

    unsigned int freeSpaceSector;
    char2ul(&buffer[SUPER_FREE_SPACE_SECTOR], &freeSpaceSector);

    unsigned int freeSpaceSize = firstBlock - freeSpaceSector;

    for (int i = freeSpaceSector; i < freeSpaceSector + freeSpaceSize; i++) {
        if (diskReadSector(disk, i, buffer) == -1)
            return -1;

        for (int j = 0; j < DISK_SECTORDATASIZE; j++) {
            int freeBit = firstZeroBit(buffer[j]);

            if(freeBit != -1) {
                unsigned int freeBlock = firstBlock +
                        (i - freeSpaceSector) * DISK_SECTORDATASIZE * 8 * sectorsPerBlock +
                        j * 8 * sectorsPerBlock +
                        freeBit * sectorsPerBlock;

                if ((freeBlock - firstBlock) / sectorsPerBlock >= numBlocks)
                    return -1;

                buffer[j] = setBitToOne(buffer[j], freeBit);
                if (diskWriteSector(disk, i, buffer) == -1)
                    return -1;

                return freeBlock;
            }
        }
    }

    return -1;
}

bool setBlockFree(Disk *d, unsigned int block) {
    unsigned char buffer[DISK_SECTORDATASIZE];
    if (diskReadSector(d, 0, buffer) == -1)
        return false;

    unsigned int sectorsPerBlock;
    char2ul(&buffer[SUPER_BLOCKSIZE], &sectorsPerBlock);
    sectorsPerBlock /= DISK_SECTORDATASIZE;

    unsigned int numBlocks;
    char2ul(&buffer[SUPER_NUM_BLOCKS], &numBlocks);

    unsigned int firstBlock;
    char2ul(&buffer[SUPER_FIRST_BLOCK_SECTOR], &firstBlock);

    unsigned int freeSpaceStartSector;
    char2ul(&buffer[SUPER_FREE_SPACE_SECTOR], &freeSpaceStartSector);

    if ((block - firstBlock) / sectorsPerBlock >= numBlocks)
        return false;

    unsigned int blockFreeSpaceSector = ((block - firstBlock) / sectorsPerBlock) / (DISK_SECTORDATASIZE * 8);
    if (diskReadSector(d, blockFreeSpaceSector, buffer) == -1)
        return false;

    unsigned int blockFreeSpaceBit = ((block - firstBlock) / sectorsPerBlock) % (DISK_SECTORDATASIZE * 8);
    buffer[blockFreeSpaceBit / 8] = setBitToZero(buffer[blockFreeSpaceBit / 8], blockFreeSpaceBit % 8);

    if (diskWriteSector(d, blockFreeSpaceSector, buffer) == -1)
        return false;

    return true;
}
//} Fim - Metodos auxiliares

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
    unsigned char superblock[DISK_SECTORDATASIZE] = {0};

    ul2char(blockSize, &superblock[SUPER_BLOCKSIZE]);

    unsigned int numInodes = (diskGetSize(disk) / blockSize) / 8;

    unsigned int freeSpaceSector = inodeAreaBeginSector() + numInodes / inodeNumInodesPerSector();
    unsigned int freeSpaceSize = (diskGetSize(disk) / blockSize) / (sizeof(unsigned char) * 8 * DISK_SECTORDATASIZE);

    ul2char(freeSpaceSector, &superblock[SUPER_FREE_SPACE_SECTOR]);

    unsigned int firstBlockSector = freeSpaceSector + freeSpaceSize;
    unsigned int numBlocks = (diskGetNumSectors(disk) - firstBlockSector) / (blockSize / DISK_SECTORDATASIZE);

    ul2char(firstBlockSector, &superblock[SUPER_FIRST_BLOCK_SECTOR]);
    ul2char(numBlocks, &superblock[SUPER_NUM_BLOCKS]);

    if (diskWriteSector(disk, 0, superblock) == -1)
        return -1;

    unsigned char freeSpace[DISK_SECTORDATASIZE] = {0};
    for (int i = 0; i < freeSpaceSize; i++) {
        if (diskWriteSector(disk, freeSpaceSector + i, freeSpace) == -1)
            return -1;
    }

    return numBlocks > 0 ? numBlocks : -1;
}

int openFile(Disk *disk, const char *path) {
    return -1;
}

int readFile(int fd, char *buf, unsigned int nbytes) {
    if(fd < 0 || fd >= MAX_FDS) return -1;
    File* file = openFiles[fd];
    if(file == NULL)
        return -1;

    unsigned int fileSize = inodeGetFileSize(file->inode);
    unsigned int bytesRead = 0;
    unsigned int currentInodeBlockNum = file->lastByteRead / file->blockSize;
    unsigned int offset = file->lastByteRead % file->blockSize;
    unsigned int currentBlock = inodeGetBlockAddr(file->inode, currentInodeBlockNum);
    unsigned char diskBuffer[DISK_SECTORDATASIZE];

    while(bytesRead < nbytes &&
          bytesRead + file->lastByteRead < fileSize &&
          currentBlock > 0) {
        unsigned int sectorsPerBlock = file->blockSize / DISK_SECTORDATASIZE;
        unsigned int firstSector = offset / DISK_SECTORDATASIZE;
        unsigned int firstByteInSector = offset % DISK_SECTORDATASIZE;

        for(int i = firstSector; i < sectorsPerBlock && bytesRead < nbytes; i++) {
            if(diskReadSector(file->disk, currentBlock + i, diskBuffer) == -1)
                return -1;

            for(int j = firstByteInSector;  j < DISK_SECTORDATASIZE &&
                bytesRead < nbytes &&
                bytesRead + file->lastByteRead < fileSize;  j++) {
                buf[bytesRead] = diskBuffer[j];
                bytesRead++;
            }

            firstByteInSector = 0;
        }

        offset = 0;
        currentInodeBlockNum++;
        currentBlock = inodeGetBlockAddr(file->inode, currentInodeBlockNum);
    }

    file->lastByteRead += bytesRead;

    return bytesRead;
}

int writeFile(int fd, const char *buf, unsigned int nbytes) {
    if(fd <= 0 || fd > MAX_FDS)
        return -1;

    File* file = openFiles[fd];
    if(!file)
        return -1;

    unsigned int fileSize = inodeGetFileSize(file->inode);
    unsigned int bytesWritten = 0;
    unsigned int currentInodeBlockNum = file->lastByteRead / file->blockSize;
    unsigned int offset = file->lastByteRead % file->blockSize;
    unsigned int currentBlock = inodeGetBlockAddr(file->inode, currentInodeBlockNum);
    unsigned char diskBuffer[DISK_SECTORDATASIZE];

    while(bytesWritten < nbytes) {
        unsigned int sectorsPerBlock = file->blockSize / DISK_SECTORDATASIZE;
        unsigned int firstSector = offset / DISK_SECTORDATASIZE;
        unsigned int firstByteInSector = offset % DISK_SECTORDATASIZE;

        if (currentBlock == 0) {
            currentBlock = findFreeBlock(file->disk);

            if(currentBlock == -1)
                break;

            if(inodeAddBlock(file->inode, currentBlock) == -1) {
                setBlockFree(file->disk, currentBlock);
                break;
            }
        }

        for (int i = firstSector; i < sectorsPerBlock && bytesWritten < nbytes; i++) {
            if(diskReadSector(file->disk, currentBlock + i, diskBuffer) == -1) return -1;

            for (int j = firstByteInSector; j < DISK_SECTORDATASIZE && bytesWritten < nbytes; j++) {
                diskBuffer[j] = buf[bytesWritten];
                bytesWritten++;
            }

            if (diskWriteSector(file->disk, currentBlock + i, diskBuffer) == -1)
                return -1;
            firstByteInSector = 0;
        }

        offset = 0;
        currentInodeBlockNum++;
        currentBlock = inodeGetBlockAddr(file->inode, currentInodeBlockNum);
    }

    file->lastByteRead += bytesWritten;
    if(file->lastByteRead >= fileSize) {
        inodeSetFileSize(file->inode, file->lastByteRead);
        inodeSave(file->inode);
    }

    return bytesWritten;
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
    return -1;
}

int readDir(int fd, char *filename, unsigned int *inumber) {
    if(fd < 0 || fd >= MAX_FDS) return -1;
    File* file = openFiles[fd];

    if(file == NULL || inodeGetFileType(file->inode) != FILETYPE_DIR)
        return -1;

    DirectoryEntry entry;
    int bytesRead = readFile(fd, (char*) &entry, sizeof(DirectoryEntry));

    if(bytesRead == -1)
        return -1;
    if(bytesRead < sizeof(DirectoryEntry))
        return 0;

    strcpy(filename, entry.path);
    *inumber = entry.numInode;
    return 1;
}

int link(int fd, const char *filename, unsigned int inumber) {
    if (fd <= 0 || fd > MAX_FDS)
        return -1;
    File* dir = openFiles[fd];

    if(!dir || inodeGetFileType(dir->inode) != FILETYPE_DIR)
        return -1;

    Inode* inodeToLink = inodeLoad(inumber, dir->disk);
    if (!inodeToLink)
        return -1;

    DirectoryEntry entry;
    strcpy(entry.path, filename);
    entry.numInode = inumber;

    unsigned int previousCurrentByte = dir->lastByteRead;
    unsigned int previousDirSize = inodeGetFileSize(dir->inode);
    dir->lastByteRead = previousDirSize;

    int bytesWritten = writeFile(fd, (const char*) &entry, sizeof(DirectoryEntry));
    dir->lastByteRead = previousCurrentByte;

    if (bytesWritten != sizeof(DirectoryEntry)) {
        inodeSetFileSize(dir->inode, previousDirSize);
        inodeSave(dir->inode);

        free(inodeToLink);
        return -1;
    }

    unsigned int previousRefCount = inodeGetRefCount(inodeToLink);
    inodeSetRefCount(inodeToLink, previousRefCount + 1);

    inodeSave(inodeToLink);
    free(inodeToLink);
    return 0;
}

int unlink(int fd, const char *filename) {
    if (fd <= 0 || fd > MAX_FDS)
        return -1;
    File* dir = openFiles[fd];

    if (!dir|| inodeGetFileType(dir->inode) != FILETYPE_DIR)
        return -1;

    unsigned int previousCurrentByte = dir->lastByteRead;
    dir->lastByteRead = 0;

    DirectoryEntry entry;
    unsigned int inumber = 0;
    while (readFile(fd, (char*) &entry, sizeof(DirectoryEntry)) == sizeof(DirectoryEntry)) {
        if (strcmp(entry.path, filename) == 0) {
            inumber = entry.numInode;

            unsigned int currentEntryByte = dir->lastByteRead;
            unsigned int nextEntryByte = dir->lastByteRead + sizeof(DirectoryEntry);

            dir->lastByteRead = nextEntryByte;
            while(readFile(fd, (char*) &entry, sizeof(DirectoryEntry)) == sizeof(DirectoryEntry)) {
                dir->lastByteRead = currentEntryByte;
                writeFile(fd, (char*) &entry, sizeof(DirectoryEntry));

                currentEntryByte += sizeof(DirectoryEntry);
                nextEntryByte += sizeof(DirectoryEntry);

                dir->lastByteRead = nextEntryByte;
            }

            unsigned int previousDirSize = inodeGetFileSize(dir->inode);
            inodeSetFileSize(dir->inode, previousDirSize - sizeof(DirectoryEntry));
            break;
        }
    }

    dir->lastByteRead = previousCurrentByte;
    if (inumber == 0)
        return -1;

    Inode* inodeToUnlink = inodeLoad(inumber, dir->disk);
    if (!inodeToUnlink)
        return -1;

    unsigned int previousRefCount = inodeGetRefCount(inodeToUnlink);
    inodeSetRefCount(inodeToUnlink, previousRefCount - 1);

    if (previousRefCount == 1) {
        unsigned int blockCount = 0;
        unsigned int currentBlock = inodeGetBlockAddr(inodeToUnlink, blockCount);

        while (currentBlock > 0) {
            setBlockFree(dir->disk, currentBlock);
            blockCount++;
            currentBlock = inodeGetBlockAddr(inodeToUnlink, blockCount);
        }

        inodeClear(inodeToUnlink);
    }

    inodeSave(inodeToUnlink);
    free(inodeToUnlink);

    return 0;
}

int closeDir(int fd) {
    return -1;
}
