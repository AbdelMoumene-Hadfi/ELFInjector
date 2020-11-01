;;nasm -f elf64 -o payload.o payload.s && ld -o payload payload.o
section .text
  global _start

_start:
  ;; save cpu state
  push rax
  push rdi
  push rsi
  push rdx

  ;; write msg to stdout
  mov rax,1                     ; [1] - sys_write
  mov rdi,1                     ; 0 = stdin / 1 = stdout / 2 = stderr
  lea rsi,[rel msg]             ; pointer(mem address) to msg (*char[])
  mov rdx, msg_end - msg        ; msg size
  syscall                       ; calls the function stored in rax

  ;; restore cpu state
  pop rdx
  pop rsi
  pop rdi
  pop rax

  ;; jump to _main
  jmp -0x00000000  ; address changed during injection

align 8
  msg     db 'payload running ...',0x0a,0
  msg_end db 0x0
