/*
*  myfs.c - Implementacao de fimcpes do sistema de arquivos
*
*  Autores: Lucas de Pace, André Caetano e Cristiano
*  Projeto: Trabalho Pratico II - Sistemas Operacionais
*  Organizacao: Universidade Federal de Juiz de Fora
*  Departamento: Dep. Ciencia da Computacao
*
*  
*/

#include "myfs.h"
#include "disk.h"


/*
char fsid; // Identificador do tipo de sistema de arquivos
char *fsname; // Nome do tipo de sistema de arquivos
int (*isidleFn) (Disk *d);
int (*formatFn) (Disk *d, unsigned int blockSize);
int (*openFn) (Disk *d, const char *path);
int (*readFn) (int fd, char *buf, unsigned int nbytes);
int (*writeFn) (int fd, const char *buf, unsigned int nbytes);
int (*closeFn) (int fd);
int (*opendirFn) (Disk *d, const char *path);
int (*readdirFn) (int fd, char *filename, unsigned int *inumber,
unsigned int enumber);
int (*linkFn) (int fd, const char *filename, unsigned int inumber);
int (*unlinkFn) (int fd, const char *filename);
int (*closedirFn) (int fd);

 */


int installMyFS()
{
}
