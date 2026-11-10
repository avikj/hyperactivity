.intel_syntax noprefix
.text
_start:
  mov eax, 9
  xor edi, edi
  mov esi, 0x40000000
  mov edx, 3
  mov r10d, 0x4022
  mov r8, -1
  xor r9d, r9d
  syscall
  sub rsp, 32
  mov qword ptr [rsp], 0
  mov [rsp+8], rax
  lea rcx, [rax+0x40000000]
  mov [rsp+16], rcx
  lea rcx, [rsp-0x700000]
  mov [rsp+24], rcx
  mov rdx, rsp
  xor edi, edi
  xor esi, esi
  call blob
  cmp byte ptr [rsp], 0
  jne ovf
  xor edi, edi
  mov eax, 231
  syscall
ovf:
  mov edi, 3
  mov eax, 231
  syscall
blob:
