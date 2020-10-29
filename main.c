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
void get_elf_header(void* map) {
    Elf64_Ehdr* elf_hdr = (Elf64_Ehdr *)map;
    Elf64_Off phoff = elf_hdr->e_phoff;
    Elf64_Half phnum = elf_hdr->e_phnum;
    printf("[+] Entry Point : 0x%lx \n",elf_hdr->e_entry);
    printf("[+] phoff : %ld \n",phoff);
    printf("[+] phnum : %d \n",phnum);
    Elf64_Phdr* elf_seg = (Elf64_Phdr *)((void*)elf_hdr+phoff);
    printf("[+] type | flags | file offset | virtual address | physical address | size in file | size in memory | alignement\n");
    for (int i=0;i<phnum;i++) {
      printf(" 0x%016x | 0x%016x | 0x%016ld | 0x%016lx | 0x%016lx | 0x%016lx | 0x%016lx | 0x%016lx\n",elf_seg->p_type,elf_seg->p_flags,elf_seg->p_offset,elf_seg->p_vaddr,elf_seg->p_paddr,elf_seg->p_filesz,elf_seg->p_memsz,elf_seg->p_align);
      elf_seg = (Elf64_Phdr *)((void*)elf_seg+elf_hdr->e_phentsize);
    }

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
  get_elf_header(map_payl);
  return EXIT_SUCCESS;

}
