
main:     file format elf32-i386

Disassembly of section .init:

08048274 <_init>:
 8048274:	55                   	push   %ebp
 8048275:	89 e5                	mov    %esp,%ebp
 8048277:	53                   	push   %ebx
 8048278:	83 ec 04             	sub    $0x4,%esp
 804827b:	e8 00 00 00 00       	call   8048280 <_init+0xc>
 8048280:	5b                   	pop    %ebx
 8048281:	81 c3 2c 13 00 00    	add    $0x132c,%ebx
 8048287:	8b 93 fc ff ff ff    	mov    -0x4(%ebx),%edx
 804828d:	85 d2                	test   %edx,%edx
 804828f:	74 05                	je     8048296 <_init+0x22>
 8048291:	e8 1e 00 00 00       	call   80482b4 <__gmon_start__@plt>
 8048296:	e8 b5 00 00 00       	call   8048350 <frame_dummy>
 804829b:	e8 c0 01 00 00       	call   8048460 <__do_global_ctors_aux>
 80482a0:	58                   	pop    %eax
 80482a1:	5b                   	pop    %ebx
 80482a2:	c9                   	leave  
 80482a3:	c3                   	ret    
Disassembly of section .plt:

080482a4 <__gmon_start__@plt-0x10>:
 80482a4:	ff 35 b0 95 04 08    	pushl  0x80495b0
 80482aa:	ff 25 b4 95 04 08    	jmp    *0x80495b4
 80482b0:	00 00                	add    %al,(%eax)
	...

080482b4 <__gmon_start__@plt>:
 80482b4:	ff 25 b8 95 04 08    	jmp    *0x80495b8
 80482ba:	68 00 00 00 00       	push   $0x0
 80482bf:	e9 e0 ff ff ff       	jmp    80482a4 <_init+0x30>

080482c4 <__libc_start_main@plt>:
 80482c4:	ff 25 bc 95 04 08    	jmp    *0x80495bc
 80482ca:	68 08 00 00 00       	push   $0x8
 80482cf:	e9 d0 ff ff ff       	jmp    80482a4 <_init+0x30>

080482d4 <puts@plt>:
 80482d4:	ff 25 c0 95 04 08    	jmp    *0x80495c0
 80482da:	68 10 00 00 00       	push   $0x10
 80482df:	e9 c0 ff ff ff       	jmp    80482a4 <_init+0x30>
Disassembly of section .text:

080482f0 <_start>:
 80482f0:	31 ed                	xor    %ebp,%ebp
 80482f2:	5e                   	pop    %esi
 80482f3:	89 e1                	mov    %esp,%ecx
 80482f5:	83 e4 f0             	and    $0xfffffff0,%esp
 80482f8:	50                   	push   %eax
 80482f9:	54                   	push   %esp
 80482fa:	52                   	push   %edx
 80482fb:	68 f0 83 04 08       	push   $0x80483f0
 8048300:	68 00 84 04 08       	push   $0x8048400
 8048305:	51                   	push   %ecx
 8048306:	56                   	push   %esi
 8048307:	68 74 83 04 08       	push   $0x8048374
 804830c:	e8 b3 ff ff ff       	call   80482c4 <__libc_start_main@plt>
 8048311:	f4                   	hlt    
 8048312:	90                   	nop    
 8048313:	90                   	nop    
 8048314:	90                   	nop    
 8048315:	90                   	nop    
 8048316:	90                   	nop    
 8048317:	90                   	nop    
 8048318:	90                   	nop    
 8048319:	90                   	nop    
 804831a:	90                   	nop    
 804831b:	90                   	nop    
 804831c:	90                   	nop    
 804831d:	90                   	nop    
 804831e:	90                   	nop    
 804831f:	90                   	nop    

08048320 <__do_global_dtors_aux>:
 8048320:	55                   	push   %ebp
 8048321:	89 e5                	mov    %esp,%ebp
 8048323:	83 ec 08             	sub    $0x8,%esp
 8048326:	80 3d d4 95 04 08 00 	cmpb   $0x0,0x80495d4
 804832d:	74 0c                	je     804833b <__do_global_dtors_aux+0x1b>
 804832f:	eb 1c                	jmp    804834d <__do_global_dtors_aux+0x2d>
 8048331:	83 c0 04             	add    $0x4,%eax
 8048334:	a3 cc 95 04 08       	mov    %eax,0x80495cc
 8048339:	ff d2                	call   *%edx
 804833b:	a1 cc 95 04 08       	mov    0x80495cc,%eax
 8048340:	8b 10                	mov    (%eax),%edx
 8048342:	85 d2                	test   %edx,%edx
 8048344:	75 eb                	jne    8048331 <__do_global_dtors_aux+0x11>
 8048346:	c6 05 d4 95 04 08 01 	movb   $0x1,0x80495d4
 804834d:	c9                   	leave  
 804834e:	c3                   	ret    
 804834f:	90                   	nop    

08048350 <frame_dummy>:
 8048350:	55                   	push   %ebp
 8048351:	89 e5                	mov    %esp,%ebp
 8048353:	83 ec 08             	sub    $0x8,%esp
 8048356:	a1 d4 94 04 08       	mov    0x80494d4,%eax
 804835b:	85 c0                	test   %eax,%eax
 804835d:	74 12                	je     8048371 <frame_dummy+0x21>
 804835f:	b8 00 00 00 00       	mov    $0x0,%eax
 8048364:	85 c0                	test   %eax,%eax
 8048366:	74 09                	je     8048371 <frame_dummy+0x21>
 8048368:	c7 04 24 d4 94 04 08 	movl   $0x80494d4,(%esp)
 804836f:	ff d0                	call   *%eax
 8048371:	c9                   	leave  
 8048372:	c3                   	ret    
 8048373:	90                   	nop    

08048374 <main>:
 8048374:	8d 4c 24 04          	lea    0x4(%esp),%ecx
 8048378:	83 e4 f0             	and    $0xfffffff0,%esp
 804837b:	ff 71 fc             	pushl  -0x4(%ecx)
 804837e:	55                   	push   %ebp
 804837f:	89 e5                	mov    %esp,%ebp
 8048381:	51                   	push   %ecx
 8048382:	83 ec 14             	sub    $0x14,%esp
 8048385:	c7 04 24 b0 84 04 08 	movl   $0x80484b0,(%esp)
 804838c:	e8 43 ff ff ff       	call   80482d4 <puts@plt>
 8048391:	c7 04 24 00 00 00 00 	movl   $0x0,(%esp)
 8048398:	c7 44 24 04 00 00 00 	movl   $0x0,0x4(%esp)
 804839f:	00 
 80483a0:	e8 1a 00 00 00       	call   80483bf <break_fn>
 80483a5:	c7 04 24 b6 84 04 08 	movl   $0x80484b6,(%esp)
 80483ac:	e8 23 ff ff ff       	call   80482d4 <puts@plt>
 80483b1:	b8 00 00 00 00       	mov    $0x0,%eax
 80483b6:	83 c4 14             	add    $0x14,%esp
 80483b9:	59                   	pop    %ecx
 80483ba:	5d                   	pop    %ebp
 80483bb:	8d 61 fc             	lea    -0x4(%ecx),%esp
 80483be:	c3                   	ret    

080483bf <break_fn>:
 80483bf:	55                   	push   %ebp
 80483c0:	89 e5                	mov    %esp,%ebp
 80483c2:	83 ec 08             	sub    $0x8,%esp
 80483c5:	8b 45 08             	mov    0x8(%ebp),%eax
 80483c8:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80483cb:	8b 45 0c             	mov    0xc(%ebp),%eax
 80483ce:	89 45 fc             	mov    %eax,-0x4(%ebp)
 80483d1:	0f 38 00 00          	pshufb (%eax),%mm0
 80483d5:	00 00                	add    %al,(%eax)
 80483d7:	00 00                	add    %al,(%eax)
 80483d9:	00 00                	add    %al,(%eax)
 80483db:	0f 38 ff             	(bad)  
 80483de:	00 00                	add    %al,(%eax)
 80483e0:	00 ff                	add    %bh,%bh
 80483e2:	00 00                	add    %al,(%eax)
 80483e4:	00 c9                	add    %cl,%cl
 80483e6:	c3                   	ret    
 80483e7:	90                   	nop    
 80483e8:	90                   	nop    
 80483e9:	90                   	nop    
 80483ea:	90                   	nop    
 80483eb:	90                   	nop    
 80483ec:	90                   	nop    
 80483ed:	90                   	nop    
 80483ee:	90                   	nop    
 80483ef:	90                   	nop    

080483f0 <__libc_csu_fini>:
 80483f0:	55                   	push   %ebp
 80483f1:	89 e5                	mov    %esp,%ebp
 80483f3:	5d                   	pop    %ebp
 80483f4:	c3                   	ret    
 80483f5:	8d 74 26 00          	lea    0x0(%esi),%esi
 80483f9:	8d bc 27 00 00 00 00 	lea    0x0(%edi),%edi

08048400 <__libc_csu_init>:
 8048400:	55                   	push   %ebp
 8048401:	89 e5                	mov    %esp,%ebp
 8048403:	57                   	push   %edi
 8048404:	56                   	push   %esi
 8048405:	53                   	push   %ebx
 8048406:	e8 4f 00 00 00       	call   804845a <__i686.get_pc_thunk.bx>
 804840b:	81 c3 a1 11 00 00    	add    $0x11a1,%ebx
 8048411:	83 ec 0c             	sub    $0xc,%esp
 8048414:	e8 5b fe ff ff       	call   8048274 <_init>
 8048419:	8d bb 18 ff ff ff    	lea    -0xe8(%ebx),%edi
 804841f:	8d 83 18 ff ff ff    	lea    -0xe8(%ebx),%eax
 8048425:	29 c7                	sub    %eax,%edi
 8048427:	c1 ff 02             	sar    $0x2,%edi
 804842a:	85 ff                	test   %edi,%edi
 804842c:	74 24                	je     8048452 <__libc_csu_init+0x52>
 804842e:	31 f6                	xor    %esi,%esi
 8048430:	8b 45 10             	mov    0x10(%ebp),%eax
 8048433:	89 44 24 08          	mov    %eax,0x8(%esp)
 8048437:	8b 45 0c             	mov    0xc(%ebp),%eax
 804843a:	89 44 24 04          	mov    %eax,0x4(%esp)
 804843e:	8b 45 08             	mov    0x8(%ebp),%eax
 8048441:	89 04 24             	mov    %eax,(%esp)
 8048444:	ff 94 b3 18 ff ff ff 	call   *-0xe8(%ebx,%esi,4)
 804844b:	83 c6 01             	add    $0x1,%esi
 804844e:	39 f7                	cmp    %esi,%edi
 8048450:	75 de                	jne    8048430 <__libc_csu_init+0x30>
 8048452:	83 c4 0c             	add    $0xc,%esp
 8048455:	5b                   	pop    %ebx
 8048456:	5e                   	pop    %esi
 8048457:	5f                   	pop    %edi
 8048458:	5d                   	pop    %ebp
 8048459:	c3                   	ret    

0804845a <__i686.get_pc_thunk.bx>:
 804845a:	8b 1c 24             	mov    (%esp),%ebx
 804845d:	c3                   	ret    
 804845e:	90                   	nop    
 804845f:	90                   	nop    

08048460 <__do_global_ctors_aux>:
 8048460:	55                   	push   %ebp
 8048461:	89 e5                	mov    %esp,%ebp
 8048463:	53                   	push   %ebx
 8048464:	83 ec 04             	sub    $0x4,%esp
 8048467:	a1 c4 94 04 08       	mov    0x80494c4,%eax
 804846c:	83 f8 ff             	cmp    $0xffffffff,%eax
 804846f:	74 12                	je     8048483 <__do_global_ctors_aux+0x23>
 8048471:	31 db                	xor    %ebx,%ebx
 8048473:	ff d0                	call   *%eax
 8048475:	8b 83 c0 94 04 08    	mov    0x80494c0(%ebx),%eax
 804847b:	83 eb 04             	sub    $0x4,%ebx
 804847e:	83 f8 ff             	cmp    $0xffffffff,%eax
 8048481:	75 f0                	jne    8048473 <__do_global_ctors_aux+0x13>
 8048483:	83 c4 04             	add    $0x4,%esp
 8048486:	5b                   	pop    %ebx
 8048487:	5d                   	pop    %ebp
 8048488:	c3                   	ret    
 8048489:	90                   	nop    
 804848a:	90                   	nop    
 804848b:	90                   	nop    
Disassembly of section .fini:

0804848c <_fini>:
 804848c:	55                   	push   %ebp
 804848d:	89 e5                	mov    %esp,%ebp
 804848f:	53                   	push   %ebx
 8048490:	83 ec 04             	sub    $0x4,%esp
 8048493:	e8 00 00 00 00       	call   8048498 <_fini+0xc>
 8048498:	5b                   	pop    %ebx
 8048499:	81 c3 14 11 00 00    	add    $0x1114,%ebx
 804849f:	e8 7c fe ff ff       	call   8048320 <__do_global_dtors_aux>
 80484a4:	59                   	pop    %ecx
 80484a5:	5b                   	pop    %ebx
 80484a6:	c9                   	leave  
 80484a7:	c3                   	ret    
