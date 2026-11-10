.intel_syntax noprefix
.text
loop:
  mov rcx, [rax]
  test rcx, rcx
  jz done
  mov rsi, [rax+8]
  push qword ptr [rax+16]
  cmp rsi, 4096
  jae cellc
  test rsi, rsi
  jz c_now
  mfence
  xor eax, eax
  jmp resp
cellc:
  mov rcx, [rsi]
  cmp rcx, 1
  je c_write
  cmp rcx, 2
  je c_read
  cmp rcx, 3
  je c_sleep
  cmp rcx, 4
  je c_exit
  cmp rcx, 5
  je c_open
  cmp rcx, 6
  je c_close
  cmp rcx, 7
  je c_load
  cmp rcx, 8
  je c_store
  cmp rcx, 9
  je c_cas
  cmp rcx, 11
  je c_sys
  cmp rcx, 12
  je c_alloc
  cmp rcx, 13
  je c_load8
  cmp rcx, 14
  je c_store8
  ud2
c_now:
  sub rsp, 16
  mov eax, 228
  xor edi, edi
  mov rsi, rsp
  syscall
  mov rax, [rsp]
  mov rcx, 1000000000
  imul rax, rcx
  add rax, [rsp+8]
  add rsp, 16
  jmp resp
c_write:
  mov rdi, [rsi+8]
  mov rdx, [rsi+16]
  mov rsi, [rsi+24]
  cmp rdx, [rsi]
  jbe 1f
  mov rdx, [rsi]
1:
  add rsi, 8
  xor eax, eax
  test rdx, rdx
  jz resp
  mov eax, 1
  syscall
  test rax, rax
  jns resp
  xor eax, eax
  jmp resp
c_read:
  mov rdi, [rsi+8]
  mov rdx, [rsi+16]
  lea r9, [rdx+15]
  and r9, -8
  mov r8, r15
  add r15, r9
  add r15, 24
  cmp r15, r12
  ja over
  mov [r8], rdx
  lea rsi, [r8+8]
  xor eax, eax
  test rdx, rdx
  jz 2f
  syscall
  test rax, rax
  jns 2f
  xor eax, eax
2:
  add r9, r8
  mov qword ptr [r9], 0
  mov [r9+8], rax
  mov [r9+16], r8
  mov rax, r9
  jmp resp
c_sleep:
  mov rax, [rsi+8]
  xor edx, edx
  mov rcx, 1000000000
  div rcx
  push rdx
  push rax
  mov rdi, rsp
  xor esi, esi
  mov eax, 35
  syscall
  add rsp, 16
  xor eax, eax
  jmp resp
c_exit:
  mov rdi, [rsi+8]
  mov eax, 231
  syscall
c_open:
  mov rdx, [rsi+8]
  mov r8, [rsi+16]
  mov r9, [rsi+24]
  mov r10, [rsi+32]
  cmp rdx, [r8]
  jbe 3f
  mov rdx, [r8]
3:
  lea rcx, [rdx+8]
  and rcx, -8
  mov rdi, r15
  add r15, rcx
  cmp r15, r12
  ja over
  push rdi
  lea rsi, [r8+8]
  mov rcx, rdx
  rep movsb
  pop rdi
  mov rsi, r9
  mov rdx, r10
  mov eax, 2
  syscall
  movsxd rax, eax
  jmp resp
c_close:
  mov rdi, [rsi+8]
  mov eax, 3
  syscall
  movsxd rax, eax
  jmp resp
c_load:
  mov rax, [rsi+8]
  mov rax, [rax]
  jmp resp
c_store:
  mov rax, [rsi+8]
  mov rcx, [rsi+16]
  mov [rax], rcx
  xor eax, eax
  jmp resp
c_cas:
  mov rdi, [rsi+8]
  mov rax, [rsi+16]
  mov rcx, [rsi+24]
  lock cmpxchg [rdi], rcx
  jmp resp
c_sys:
  mov rax, [rsi+8]
  mov rdi, [rsi+16]
  mov rdx, [rsi+32]
  mov r10, [rsi+40]
  mov r8, [rsi+48]
  mov r9, [rsi+56]
  mov rsi, [rsi+24]
  syscall
  jmp resp
c_alloc:
  mov rcx, [rsi+8]
  add rcx, 7
  jc over
  and rcx, -8
  mov rax, r15
  add r15, rcx
  jc over
  cmp r15, r12
  ja over
  jmp resp
c_load8:
  mov rax, [rsi+8]
  movzx eax, byte ptr [rax]
  jmp resp
c_store8:
  mov rax, [rsi+8]
  mov rcx, [rsi+16]
  mov [rax], cl
  xor eax, eax
resp:
  pop rcx
  push rcx
  push rax
  mov rax, [rbp-8]
  add rax, r14
  call rax
  add rsp, 16
  jmp loop
done:
  xor eax, eax
  mov rsp, rbp
  pop rbp
  ret
over:
