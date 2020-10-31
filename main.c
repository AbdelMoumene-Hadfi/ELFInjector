#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <stdint.h>
#include <string.h>
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

void dump_segment(void* map) {
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

Elf64_Phdr* find_text_segm(Elf64_Phdr* elf_seg,const Elf64_Half phnum,const Elf64_Half	phentsize) {
  for (int i=0;i<phnum;i++) {
    if((elf_seg->p_type==PT_LOAD) && (elf_seg->p_flags==(PF_X|PF_R))) {
      printf("[+] .text section found in segment number : %d,offset : 0x%lx,0x%lx\n",i,elf_seg->p_offset,elf_seg->p_vaddr);
      return elf_seg;
    }
    elf_seg = (Elf64_Phdr *)((void*)elf_seg+phentsize);
  }
  return NULL;
}
Elf64_Shdr* find_text_sect(Elf64_Shdr* elf_sec,const Elf64_Half shnum,const Elf64_Half	shentsize,const void* shstrndx) {
  for (int i=0;i<shnum;i++) {
    if(strcmp((char*)((void*)shstrndx+elf_sec->sh_name),".text")==0) {
      printf("[+] .text section found at offset : 0x%lx\n",elf_sec->sh_offset);
      return elf_sec;
    }
    elf_sec = (Elf64_Shdr *)((void*)elf_sec+shentsize);
  }
  return NULL;
}

void fing_gap(void* map,uint64_t *gapsize,uint64_t *gapoff) {
    Elf64_Off gap;
    Elf64_Ehdr* elf_hdr = (Elf64_Ehdr *)map;
    Elf64_Off phoff = elf_hdr->e_phoff;
    Elf64_Half phnum = elf_hdr->e_phnum,phentsize = elf_hdr->e_phentsize;
    printf("[+] Entry Point : 0x%lx \n",elf_hdr->e_entry);
    printf("[+] phoff : %ld \n",phoff);
    printf("[+] phnum : %d \n",phnum);
    Elf64_Phdr* elf_seg = (Elf64_Phdr *)((void*)elf_hdr+phoff);
    Elf64_Phdr* text_seg = find_text_segm(elf_seg,phnum,phentsize);
    if (text_seg==NULL) {
      printf("[-] can't find segment that contains .text section\n");
      exit(EXIT_FAILURE);
    }
    Elf64_Off text_end = text_seg->p_offset+text_seg->p_filesz;
    printf("[+] .text section end : 0x%lx\n",text_end);
    elf_seg = (Elf64_Phdr *)((void*)text_seg+phentsize);
    if(elf_seg->p_type==PT_LOAD && (gap=elf_seg->p_offset-text_end)>0) {
      printf("[+] gap found at 0x%lx , gape size : 0x%lx \n",text_end,gap);
    }
    else {
      printf("[-] can't find gap\n");
      exit(EXIT_FAILURE);
    }
    *gapsize = gap;
    *gapoff = text_end ;
}

void get_text_data(void* map) {
  Elf64_Ehdr* elf_hdr = (Elf64_Ehdr *)map;
  Elf64_Off shoff = elf_hdr->e_shoff;
  Elf64_Half shnum = elf_hdr->e_shnum,shentsize = elf_hdr->e_shentsize,shstrndx=elf_hdr->e_shstrndx;
  printf("[+] Entry Point : 0x%lx \n",elf_hdr->e_entry);
  printf("[+] shoff : %ld \n",shoff);
  printf("[+] shnum : %d \n",shnum);
  printf("[+] shstrndx : %d \n",shstrndx);
  Elf64_Shdr* elf_sec = (Elf64_Shdr *)((void*)elf_hdr+shoff);
  Elf64_Shdr* sh_strtab = (Elf64_Shdr *)((void*)elf_sec+shstrndx*shentsize);
  Elf64_Shdr* text_sec = find_text_sect(elf_sec,shnum,shentsize,(void*)(map+sh_strtab->sh_offset));
  if(text_sec==NULL) {
    printf("[-] can't find .text section\n");
    exit(EXIT_FAILURE);
  }
  printf("[+] size of .text section : 0x%lx (bytes)\n",text_sec->sh_size);
}

int main(int argc,char *argv[]) {
  void *map_payl,*map_targ ;
  int payl_fd,targ_fd,payl_fsize,target_fsize;
  uint64_t gapsize,gapoff;
  if(argc !=3 ) {
    printf("[Usage] %s payload target\n",argv[0]);
    return EXIT_FAILURE;
  }
  payl_fd=open_file_map(argv[1],&payl_fsize,&map_payl);
  targ_fd=open_file_map(argv[2],&target_fsize,&map_targ);
  fing_gap(map_payl,&gapsize,&gapoff);
  get_text_data(map_payl);
  return EXIT_SUCCESS;

}
