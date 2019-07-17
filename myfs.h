/*
*  myfs.h - Funcoes que permitem a instalcao e uso de um novo sistema de arquivos
*
*  Autor: Andre Caetano, Cristiano Nascimento e Lucas de Pace
*  Projeto: Trabalho Pratico II - Sistemas Operacionais
*  Organizacao: Universidade Federal de Juiz de Fora
*  Departamento: Dep. Ciencia da Computacao
*
*
*/

#ifndef MYFS_H
#define MYFS_H

#include "disk.h"
#include "inode.h"

typedef struct file File;

typedef struct directory DirectoryEntry;

//Funcao para instalar um novo sistema de arquivos no S.O.
int installMyFS ( void );

int myfsIsIdle (Disk *disk);

int formatDisk (Disk *disk, unsigned int blockSize);

int openFile (Disk *disk, const char *path);

int readFile (int fd, char *buf, unsigned int nbytes);

int writeFile (int fd, const char *buf, unsigned int nbytes);

int closeFile (int fd);

int openDir (Disk *disk, const char *path);

int readDir (int fd, char *filename, unsigned int *inumber);

int link (int fd, const char *filename, unsigned int inumber);

int unlink (int fd, const char *filename);

int closeDir (int fd);

#endif
