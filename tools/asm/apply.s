.intel_syntax noprefix
.text
apply:
  push rbp
  mov rbp, rsp
  mov rax, [rbp+24]
  mov rcx, [rax+16]
  inc rcx
  cmp rcx, [rax+8]
  jb partial
  mov rsi, rax
  xor edi, edi
l1:
  cmp rdi, [rsi+16]
  jae d1
  push qword ptr [rsi+rdi*8+24]
  inc rdi
  jmp l1
d1:
  push qword ptr [rbp+16]
  mov rax, [rsi]
  add rax, r14
  call rax
  mov rsp, rbp
  pop rbp
  ret
partial:
  mov rsi, rax
  mov rdi, rcx
  lea rcx, [rdi*8+24]
  mov rax, r15
  add r15, rcx
  cmp r15, r12
  ja over
  mov rdx, [rsi]
  mov [rax], rdx
  mov rdx, [rsi+8]
  mov [rax+8], rdx
  mov [rax+16], rdi
  xor ecx, ecx
  lea r8, [rdi-1]
l2:
  cmp rcx, r8
  jae d2
  mov rdx, [rsi+rcx*8+24]
  mov [rax+rcx*8+24], rdx
  inc rcx
  jmp l2
d2:
  mov rdx, [rbp+16]
  mov [rax+rcx*8+24], rdx
  mov rsp, rbp
  pop rbp
  ret
over:
