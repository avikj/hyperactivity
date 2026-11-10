.intel_syntax noprefix
.text
# Two variants, by `as --defsym LOOP=1`:
#   plain (lib/elf.cub i-driver-loop): entered with the first step in rax and [rbp-8] the apply
#     routine's offset; `done` returns 0 to the stub.
#   LOOP (i-driver-round, #build-loop): entered after the encoder's prologue has pushed the
#     offsets of apply [rbp-8], copy [rbp-16], step [rbp-24], init [rbp-32]. The arena is split
#     in two halves; each round the state is copied into the other half (by the program's own
#     copy definition, which allocates there), the half just left is free again, and step runs
#     on the copy; `done` (sdone s') starts the next round with s'.
.ifdef LOOP
  push r15                  # [rbp-40] base
  mov rax, r12
  sub rax, r15
  shr rax, 1
  and rax, -8
  add rax, r15
  push rax                  # [rbp-48] mid
  push r12                  # [rbp-56] end
  push 0                    # [rbp-64] the current half (0: base..mid)
  mov r12, [rbp-48]
  mov rax, [rbp-32]
  add rax, r14
  call rax                  # init: the first state
round:
  xor qword ptr [rbp-64], 1
  jz to0
  mov r15, [rbp-48]
  mov r12, [rbp-56]
  jmp flipped
to0:
  mov r15, [rbp-40]
  mov r12, [rbp-48]
flipped:
  push rax
  mov rax, [rbp-16]
  add rax, r14
  call rax                  # copy s, into the half just entered
  mov [rsp], rax
  mov rax, [rbp-24]
  add rax, r14
  call rax                  # step s
  add rsp, 8
.endif
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
  # the buffer is n zero bytes (the arena's memory may have been used before)
  push rdi
  lea rdi, [r8+8]
  mov rcx, rdx
  xor eax, eax
  rep stosb
  pop rdi
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
  mov byte ptr [rdi], 0
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
  # n fresh zero bytes valid for the process's life: its own mapping, not the arena (which a
  # loop reuses): mmap(0, n rounded up to a page, read|write, private|anonymous, -1, 0)
  mov rsi, [rsi+8]
  add rsi, 4095
  jc over
  and rsi, -4096
  jnz 4f
  mov esi, 4096
4:
  mov eax, 9
  xor edi, edi
  mov edx, 3
  mov r10d, 0x22
  mov r8, -1
  xor r9d, r9d
  syscall
  cmp rax, -4096
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
.ifdef LOOP
  mov rax, [rax+8]
  jmp round
.else
  xor eax, eax
  mov rsp, rbp
  pop rbp
  ret
.endif
over:
