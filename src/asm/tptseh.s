.globl _MyCallSEH
.intel_syntax noprefix

	.align 16

_MyCallSEH:
	push ebp
	push edi
	push esi
	push ebx
	push offset 1f
	push fs:[0]
	mov ebp, esp
	mov fs:[0], ebp
	push 0
	push 0	# __try
	push [ebp+32]
	call [ebp+28]
	mov eax, 1
2:
	mov esp, ebp
	pop fs:[0]
	mov ecx, ebp
	call 2f
	pop ecx
	pop ebx
	pop esi
	pop edi
	pop ebp
	ret
	.align 16
1:
	mov eax, [esp+4]
	test dword ptr [eax+4], 6
	jne 1f
	mov ecx, [esp+8]
	call 2f
	mov eax, 1
	ret
1:
	push 0
	push 0
	push 1f
	push [esp+20]
	call _RtlUnwind@16
1:
	xor	eax, eax
	mov ebp, [esp+8]
	jmp 2b
	.align 16
2:
	push ebp
	mov ebp, esp
	push 1	# __finally
	push [ecx+32]
	call [ecx+28]
	leave
	ret
