#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>

int get_file_size(const int fd) {
  struct stat statbuf;
  int status ;
  if ((status=fstat(fd,&statbuf))==-1) {
    perror("[ERROR] Can't get file info");
    exit(EXIT_FAILURE);
  }
  return statbuf.st_size;
}
int open_file_map(const char* file_name,int *file_size,void **map) {
  int file_fd,size;
  if((file_fd = open(file_name, O_RDWR))==-1) {
    perror("[ERROR] Can't open file");
    exit(EXIT_FAILURE);
  }
  size = get_file_size(file_fd);

  if((*map=mmap(0,size,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED,file_fd,0))==MAP_FAILED) {
    perror("[ERROR] map ");
    exit(EXIT_FAILURE);
  }
  printf("[+] File mapped (%d bytes) at %p\n",size,map);
  *file_size=size;
  return file_fd;
}

int main(int argc,char *argv[]) {
  void *map_payl,*map_targ ;
  int payl_fd,targ_fd,payl_fsize,target_fsize;
  if(argc !=3 ) {
    printf("[Usage] %s payload target\n",argv[0]);
    return EXIT_FAILURE;
  }
  payl_fd=open_file_map(argv[1],&payl_fsize,&map_payl);
  targ_fd=open_file_map(argv[2],&target_fsize,&map_targ);

  return EXIT_SUCCESS;

}
