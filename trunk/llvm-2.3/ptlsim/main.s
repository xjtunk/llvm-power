
main:     file format elf32-i386

Disassembly of section .init:

08048254 <_init>:
 8048254:	55                   	push   %ebp
 8048255:	89 e5                	mov    %esp,%ebp
 8048257:	53                   	push   %ebx
 8048258:	83 ec 04             	sub    $0x4,%esp
 804825b:	e8 00 00 00 00       	call   8048260 <_init+0xc>
 8048260:	5b                   	pop    %ebx
 8048261:	81 c3 ec 12 00 00    	add    $0x12ec,%ebx
 8048267:	8b 93 fc ff ff ff    	mov    -0x4(%ebx),%edx
 804826d:	85 d2                	test   %edx,%edx
 804826f:	74 05                	je     8048276 <_init+0x22>
 8048271:	e8 1e 00 00 00       	call   8048294 <__gmon_start__@plt>
 8048276:	e8 a5 00 00 00       	call   8048320 <frame_dummy>
 804827b:	e8 90 01 00 00       	call   8048410 <__do_global_ctors_aux>
 8048280:	58                   	pop    %eax
 8048281:	5b                   	pop    %ebx
 8048282:	c9                   	leave  
 8048283:	c3                   	ret    
Disassembly of section .plt:

08048284 <__gmon_start__@plt-0x10>:
 8048284:	ff 35 50 95 04 08    	pushl  0x8049550
 804828a:	ff 25 54 95 04 08    	jmp    *0x8049554
 8048290:	00 00                	add    %al,(%eax)
	...

08048294 <__gmon_start__@plt>:
 8048294:	ff 25 58 95 04 08    	jmp    *0x8049558
 804829a:	68 00 00 00 00       	push   $0x0
 804829f:	e9 e0 ff ff ff       	jmp    8048284 <_init+0x30>

080482a4 <__libc_start_main@plt>:
 80482a4:	ff 25 5c 95 04 08    	jmp    *0x804955c
 80482aa:	68 08 00 00 00       	push   $0x8
 80482af:	e9 d0 ff ff ff       	jmp    8048284 <_init+0x30>
Disassembly of section .text:

080482c0 <_start>:
 80482c0:	31 ed                	xor    %ebp,%ebp
 80482c2:	5e                   	pop    %esi
 80482c3:	89 e1                	mov    %esp,%ecx
 80482c5:	83 e4 f0             	and    $0xfffffff0,%esp
 80482c8:	50                   	push   %eax
 80482c9:	54                   	push   %esp
 80482ca:	52                   	push   %edx
 80482cb:	68 a0 83 04 08       	push   $0x80483a0
 80482d0:	68 b0 83 04 08       	push   $0x80483b0
 80482d5:	51                   	push   %ecx
 80482d6:	56                   	push   %esi
 80482d7:	68 44 83 04 08       	push   $0x8048344
 80482dc:	e8 c3 ff ff ff       	call   80482a4 <__libc_start_main@plt>
 80482e1:	f4                   	hlt    
 80482e2:	90                   	nop    
 80482e3:	90                   	nop    
 80482e4:	90                   	nop    
 80482e5:	90                   	nop    
 80482e6:	90                   	nop    
 80482e7:	90                   	nop    
 80482e8:	90                   	nop    
 80482e9:	90                   	nop    
 80482ea:	90                   	nop    
 80482eb:	90                   	nop    
 80482ec:	90                   	nop    
 80482ed:	90                   	nop    
 80482ee:	90                   	nop    
 80482ef:	90                   	nop    

080482f0 <__do_global_dtors_aux>:
 80482f0:	55                   	push   %ebp
 80482f1:	89 e5                	mov    %esp,%ebp
 80482f3:	83 ec 08             	sub    $0x8,%esp
 80482f6:	80 3d 70 95 04 08 00 	cmpb   $0x0,0x8049570
 80482fd:	74 0c                	je     804830b <__do_global_dtors_aux+0x1b>
 80482ff:	eb 1c                	jmp    804831d <__do_global_dtors_aux+0x2d>
 8048301:	83 c0 04             	add    $0x4,%eax
 8048304:	a3 68 95 04 08       	mov    %eax,0x8049568
 8048309:	ff d2                	call   *%edx
 804830b:	a1 68 95 04 08       	mov    0x8049568,%eax
 8048310:	8b 10                	mov    (%eax),%edx
 8048312:	85 d2                	test   %edx,%edx
 8048314:	75 eb                	jne    8048301 <__do_global_dtors_aux+0x11>
 8048316:	c6 05 70 95 04 08 01 	movb   $0x1,0x8049570
 804831d:	c9                   	leave  
 804831e:	c3                   	ret    
 804831f:	90                   	nop    

08048320 <frame_dummy>:
 8048320:	55                   	push   %ebp
 8048321:	89 e5                	mov    %esp,%ebp
 8048323:	83 ec 08             	sub    $0x8,%esp
 8048326:	a1 74 94 04 08       	mov    0x8049474,%eax
 804832b:	85 c0                	test   %eax,%eax
 804832d:	74 12                	je     8048341 <frame_dummy+0x21>
 804832f:	b8 00 00 00 00       	mov    $0x0,%eax
 8048334:	85 c0                	test   %eax,%eax
 8048336:	74 09                	je     8048341 <frame_dummy+0x21>
 8048338:	c7 04 24 74 94 04 08 	movl   $0x8049474,(%esp)
 804833f:	ff d0                	call   *%eax
 8048341:	c9                   	leave  
 8048342:	c3                   	ret    
 8048343:	90                   	nop    

08048344 <main>:
 8048344:	8d 4c 24 04          	lea    0x4(%esp),%ecx
 8048348:	83 e4 f0             	and    $0xfffffff0,%esp
 804834b:	ff 71 fc             	pushl  -0x4(%ecx)
 804834e:	55                   	push   %ebp
 804834f:	89 e5                	mov    %esp,%ebp
 8048351:	51                   	push   %ecx
 8048352:	83 ec 14             	sub    $0x14,%esp
 8048355:	c7 04 24 00 00 00 00 	movl   $0x0,(%esp)
 804835c:	c7 44 24 04 00 00 00 	movl   $0x0,0x4(%esp)
 8048363:	00 
 8048364:	e8 0e 00 00 00       	call   8048377 <break_fn>
 8048369:	b8 00 00 00 00       	mov    $0x0,%eax
 804836e:	83 c4 14             	add    $0x14,%esp
 8048371:	59                   	pop    %ecx
 8048372:	5d                   	pop    %ebp
 8048373:	8d 61 fc             	lea    -0x4(%ecx),%esp
 8048376:	c3                   	ret    

08048377 <break_fn>:
 8048377:	55                   	push   %ebp
 8048378:	89 e5                	mov    %esp,%ebp
 804837a:	83 ec 08             	sub    $0x8,%esp
 804837d:	8b 45 08             	mov    0x8(%ebp),%eax
 8048380:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8048383:	8b 45 0c             	mov    0xc(%ebp),%eax
 8048386:	89 45 fc             	mov    %eax,-0x4(%ebp)
 8048389:	0f 3f                	(bad)  
 804838b:	2b 00                	sub    (%eax),%eax
 804838d:	00 00                	add    %al,(%eax)
 804838f:	c9                   	leave  
 8048390:	c3                   	ret    
 8048391:	90                   	nop    
 8048392:	90                   	nop    
 8048393:	90                   	nop    
 8048394:	90                   	nop    
 8048395:	90                   	nop    
 8048396:	90                   	nop    
 8048397:	90                   	nop    
 8048398:	90                   	nop    
 8048399:	90                   	nop    
 804839a:	90                   	nop    
 804839b:	90                   	nop    
 804839c:	90                   	nop    
 804839d:	90                   	nop    
 804839e:	90                   	nop    
 804839f:	90                   	nop    

080483a0 <__libc_csu_fini>:
 80483a0:	55                   	push   %ebp
 80483a1:	89 e5                	mov    %esp,%ebp
 80483a3:	5d                   	pop    %ebp
 80483a4:	c3                   	ret    
 80483a5:	8d 74 26 00          	lea    0x0(%esi),%esi
 80483a9:	8d bc 27 00 00 00 00 	lea    0x0(%edi),%edi

080483b0 <__libc_csu_init>:
 80483b0:	55                   	push   %ebp
 80483b1:	89 e5                	mov    %esp,%ebp
 80483b3:	57                   	push   %edi
 80483b4:	56                   	push   %esi
 80483b5:	53                   	push   %ebx
 80483b6:	e8 4f 00 00 00       	call   804840a <__i686.get_pc_thunk.bx>
 80483bb:	81 c3 91 11 00 00    	add    $0x1191,%ebx
 80483c1:	83 ec 0c             	sub    $0xc,%esp
 80483c4:	e8 8b fe ff ff       	call   8048254 <_init>
 80483c9:	8d bb 18 ff ff ff    	lea    -0xe8(%ebx),%edi
 80483cf:	8d 83 18 ff ff ff    	lea    -0xe8(%ebx),%eax
 80483d5:	29 c7                	sub    %eax,%edi
 80483d7:	c1 ff 02             	sar    $0x2,%edi
 80483da:	85 ff                	test   %edi,%edi
 80483dc:	74 24                	je     8048402 <__libc_csu_init+0x52>
 80483de:	31 f6                	xor    %esi,%esi
 80483e0:	8b 45 10             	mov    0x10(%ebp),%eax
 80483e3:	89 44 24 08          	mov    %eax,0x8(%esp)
 80483e7:	8b 45 0c             	mov    0xc(%ebp),%eax
 80483ea:	89 44 24 04          	mov    %eax,0x4(%esp)
 80483ee:	8b 45 08             	mov    0x8(%ebp),%eax
 80483f1:	89 04 24             	mov    %eax,(%esp)
 80483f4:	ff 94 b3 18 ff ff ff 	call   *-0xe8(%ebx,%esi,4)
 80483fb:	83 c6 01             	add    $0x1,%esi
 80483fe:	39 f7                	cmp    %esi,%edi
 8048400:	75 de                	jne    80483e0 <__libc_csu_init+0x30>
 8048402:	83 c4 0c             	add    $0xc,%esp
 8048405:	5b                   	pop    %ebx
 8048406:	5e                   	pop    %esi
 8048407:	5f                   	pop    %edi
 8048408:	5d                   	pop    %ebp
 8048409:	c3                   	ret    

0804840a <__i686.get_pc_thunk.bx>:
 804840a:	8b 1c 24             	mov    (%esp),%ebx
 804840d:	c3                   	ret    
 804840e:	90                   	nop    
 804840f:	90                   	nop    

08048410 <__do_global_ctors_aux>:
 8048410:	55                   	push   %ebp
 8048411:	89 e5                	mov    %esp,%ebp
 8048413:	53                   	push   %ebx
 8048414:	83 ec 04             	sub    $0x4,%esp
 8048417:	a1 64 94 04 08       	mov    0x8049464,%eax
 804841c:	83 f8 ff             	cmp    $0xffffffff,%eax
 804841f:	74 12                	je     8048433 <__do_global_ctors_aux+0x23>
 8048421:	31 db                	xor    %ebx,%ebx
 8048423:	ff d0                	call   *%eax
 8048425:	8b 83 60 94 04 08    	mov    0x8049460(%ebx),%eax
 804842b:	83 eb 04             	sub    $0x4,%ebx
 804842e:	83 f8 ff             	cmp    $0xffffffff,%eax
 8048431:	75 f0                	jne    8048423 <__do_global_ctors_aux+0x13>
 8048433:	83 c4 04             	add    $0x4,%esp
 8048436:	5b                   	pop    %ebx
 8048437:	5d                   	pop    %ebp
 8048438:	c3                   	ret    
 8048439:	90                   	nop    
 804843a:	90                   	nop    
 804843b:	90                   	nop    
Disassembly of section .fini:

0804843c <_fini>:
 804843c:	55                   	push   %ebp
 804843d:	89 e5                	mov    %esp,%ebp
 804843f:	53                   	push   %ebx
 8048440:	83 ec 04             	sub    $0x4,%esp
 8048443:	e8 00 00 00 00       	call   8048448 <_fini+0xc>
 8048448:	5b                   	pop    %ebx
 8048449:	81 c3 04 11 00 00    	add    $0x1104,%ebx
 804844f:	e8 9c fe ff ff       	call   80482f0 <__do_global_dtors_aux>
 8048454:	59                   	pop    %ecx
 8048455:	5b                   	pop    %ebx
 8048456:	c9                   	leave  
 8048457:	c3                   	ret    
