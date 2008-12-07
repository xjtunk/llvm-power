
main:     file format elf32-i386

Disassembly of section .init:

08048278 <_init>:
 8048278:	55                   	push   %ebp
 8048279:	89 e5                	mov    %esp,%ebp
 804827b:	83 ec 08             	sub    $0x8,%esp
 804827e:	e8 61 00 00 00       	call   80482e4 <call_gmon_start>
 8048283:	e8 b4 00 00 00       	call   804833c <frame_dummy>
 8048288:	e8 1b 02 00 00       	call   80484a8 <__do_global_ctors_aux>
 804828d:	c9                   	leave  
 804828e:	c3                   	ret    
Disassembly of section .plt:

08048290 <__libc_start_main@plt-0x10>:
 8048290:	ff 35 ec 95 04 08    	pushl  0x80495ec
 8048296:	ff 25 f0 95 04 08    	jmp    *0x80495f0
 804829c:	00 00                	add    %al,(%eax)
	...

080482a0 <__libc_start_main@plt>:
 80482a0:	ff 25 f4 95 04 08    	jmp    *0x80495f4
 80482a6:	68 00 00 00 00       	push   $0x0
 80482ab:	e9 e0 ff ff ff       	jmp    8048290 <_init+0x18>

080482b0 <printf@plt>:
 80482b0:	ff 25 f8 95 04 08    	jmp    *0x80495f8
 80482b6:	68 08 00 00 00       	push   $0x8
 80482bb:	e9 d0 ff ff ff       	jmp    8048290 <_init+0x18>
Disassembly of section .text:

080482c0 <_start>:
 80482c0:	31 ed                	xor    %ebp,%ebp
 80482c2:	5e                   	pop    %esi
 80482c3:	89 e1                	mov    %esp,%ecx
 80482c5:	83 e4 f0             	and    $0xfffffff0,%esp
 80482c8:	50                   	push   %eax
 80482c9:	54                   	push   %esp
 80482ca:	52                   	push   %edx
 80482cb:	68 64 84 04 08       	push   $0x8048464
 80482d0:	68 10 84 04 08       	push   $0x8048410
 80482d5:	51                   	push   %ecx
 80482d6:	56                   	push   %esi
 80482d7:	68 68 83 04 08       	push   $0x8048368
 80482dc:	e8 bf ff ff ff       	call   80482a0 <__libc_start_main@plt>
 80482e1:	f4                   	hlt    
 80482e2:	90                   	nop    
 80482e3:	90                   	nop    

080482e4 <call_gmon_start>:
 80482e4:	55                   	push   %ebp
 80482e5:	89 e5                	mov    %esp,%ebp
 80482e7:	53                   	push   %ebx
 80482e8:	e8 00 00 00 00       	call   80482ed <call_gmon_start+0x9>
 80482ed:	5b                   	pop    %ebx
 80482ee:	81 c3 fb 12 00 00    	add    $0x12fb,%ebx
 80482f4:	52                   	push   %edx
 80482f5:	8b 83 fc ff ff ff    	mov    0xfffffffc(%ebx),%eax
 80482fb:	85 c0                	test   %eax,%eax
 80482fd:	74 02                	je     8048301 <call_gmon_start+0x1d>
 80482ff:	ff d0                	call   *%eax
 8048301:	58                   	pop    %eax
 8048302:	5b                   	pop    %ebx
 8048303:	c9                   	leave  
 8048304:	c3                   	ret    
 8048305:	90                   	nop    
 8048306:	90                   	nop    
 8048307:	90                   	nop    

08048308 <__do_global_dtors_aux>:
 8048308:	55                   	push   %ebp
 8048309:	89 e5                	mov    %esp,%ebp
 804830b:	83 ec 08             	sub    $0x8,%esp
 804830e:	80 3d 0c 96 04 08 00 	cmpb   $0x0,0x804960c
 8048315:	74 0f                	je     8048326 <__do_global_dtors_aux+0x1e>
 8048317:	eb 1f                	jmp    8048338 <__do_global_dtors_aux+0x30>
 8048319:	8d 76 00             	lea    0x0(%esi),%esi
 804831c:	83 c0 04             	add    $0x4,%eax
 804831f:	a3 04 96 04 08       	mov    %eax,0x8049604
 8048324:	ff d2                	call   *%edx
 8048326:	a1 04 96 04 08       	mov    0x8049604,%eax
 804832b:	8b 10                	mov    (%eax),%edx
 804832d:	85 d2                	test   %edx,%edx
 804832f:	75 eb                	jne    804831c <__do_global_dtors_aux+0x14>
 8048331:	c6 05 0c 96 04 08 01 	movb   $0x1,0x804960c
 8048338:	c9                   	leave  
 8048339:	c3                   	ret    
 804833a:	89 f6                	mov    %esi,%esi

0804833c <frame_dummy>:
 804833c:	55                   	push   %ebp
 804833d:	89 e5                	mov    %esp,%ebp
 804833f:	83 ec 08             	sub    $0x8,%esp
 8048342:	a1 18 95 04 08       	mov    0x8049518,%eax
 8048347:	85 c0                	test   %eax,%eax
 8048349:	74 19                	je     8048364 <frame_dummy+0x28>
 804834b:	b8 00 00 00 00       	mov    $0x0,%eax
 8048350:	85 c0                	test   %eax,%eax
 8048352:	74 10                	je     8048364 <frame_dummy+0x28>
 8048354:	83 ec 0c             	sub    $0xc,%esp
 8048357:	68 18 95 04 08       	push   $0x8049518
 804835c:	ff d0                	call   *%eax
 804835e:	83 c4 10             	add    $0x10,%esp
 8048361:	8d 76 00             	lea    0x0(%esi),%esi
 8048364:	c9                   	leave  
 8048365:	c3                   	ret    
 8048366:	90                   	nop    
 8048367:	90                   	nop    

08048368 <main>:
 8048368:	55                   	push   %ebp
 8048369:	89 e5                	mov    %esp,%ebp
 804836b:	83 ec 08             	sub    $0x8,%esp
 804836e:	83 e4 f0             	and    $0xfffffff0,%esp
 8048371:	b8 00 00 00 00       	mov    $0x0,%eax
 8048376:	83 c0 0f             	add    $0xf,%eax
 8048379:	83 c0 0f             	add    $0xf,%eax
 804837c:	c1 e8 04             	shr    $0x4,%eax
 804837f:	c1 e0 04             	shl    $0x4,%eax
 8048382:	29 c4                	sub    %eax,%esp
 8048384:	83 ec 0c             	sub    $0xc,%esp
 8048387:	68 f0 84 04 08       	push   $0x80484f0
 804838c:	e8 1f ff ff ff       	call   80482b0 <printf@plt>
 8048391:	83 c4 10             	add    $0x10,%esp
 8048394:	83 ec 08             	sub    $0x8,%esp
 8048397:	6a 00                	push   $0x0
 8048399:	6a 00                	push   $0x0
 804839b:	e8 1a 00 00 00       	call   80483ba <break_fn>
 80483a0:	83 c4 10             	add    $0x10,%esp
 80483a3:	83 ec 0c             	sub    $0xc,%esp
 80483a6:	68 f7 84 04 08       	push   $0x80484f7
 80483ab:	e8 00 ff ff ff       	call   80482b0 <printf@plt>
 80483b0:	83 c4 10             	add    $0x10,%esp
 80483b3:	b8 00 00 00 00       	mov    $0x0,%eax
 80483b8:	c9                   	leave  
 80483b9:	c3                   	ret    

080483ba <break_fn>:
 80483ba:	55                   	push   %ebp
 80483bb:	89 e5                	mov    %esp,%ebp
 80483bd:	83 ec 04             	sub    $0x4,%esp
 80483c0:	0f 38 00 00          	pshufb (%eax),%mm0
 80483c4:	00 00                	add    %al,(%eax)
 80483c6:	00 00                	add    %al,(%eax)
 80483c8:	00 00                	add    %al,(%eax)
 80483ca:	c7 45 fc 00 00 00 00 	movl   $0x0,0xfffffffc(%ebp)
 80483d1:	81 7d fc e7 03 00 00 	cmpl   $0x3e7,0xfffffffc(%ebp)
 80483d8:	7f 07                	jg     80483e1 <break_fn+0x27>
 80483da:	8d 45 fc             	lea    0xfffffffc(%ebp),%eax
 80483dd:	ff 00                	incl   (%eax)
 80483df:	eb f0                	jmp    80483d1 <break_fn+0x17>
 80483e1:	0f 38 ff             	(bad)  
 80483e4:	00 00                	add    %al,(%eax)
 80483e6:	00 ff                	add    %bh,%bh
 80483e8:	00 00                	add    %al,(%eax)
 80483ea:	00 c7                	add    %al,%bh
 80483ec:	45                   	inc    %ebp
 80483ed:	fc                   	cld    
 80483ee:	00 00                	add    %al,(%eax)
 80483f0:	00 00                	add    %al,(%eax)
 80483f2:	81 7d fc e7 03 00 00 	cmpl   $0x3e7,0xfffffffc(%ebp)
 80483f9:	7f 07                	jg     8048402 <break_fn+0x48>
 80483fb:	8d 45 fc             	lea    0xfffffffc(%ebp),%eax
 80483fe:	ff 00                	incl   (%eax)
 8048400:	eb f0                	jmp    80483f2 <break_fn+0x38>
 8048402:	0f 38 f0             	(bad)  
 8048405:	00 00                	add    %al,(%eax)
 8048407:	00 f0                	add    %dh,%al
 8048409:	00 00                	add    %al,(%eax)
 804840b:	00 c9                	add    %cl,%cl
 804840d:	c3                   	ret    
 804840e:	90                   	nop    
 804840f:	90                   	nop    

08048410 <__libc_csu_init>:
 8048410:	55                   	push   %ebp
 8048411:	89 e5                	mov    %esp,%ebp
 8048413:	57                   	push   %edi
 8048414:	56                   	push   %esi
 8048415:	53                   	push   %ebx
 8048416:	83 ec 0c             	sub    $0xc,%esp
 8048419:	e8 00 00 00 00       	call   804841e <__libc_csu_init+0xe>
 804841e:	5b                   	pop    %ebx
 804841f:	81 c3 ca 11 00 00    	add    $0x11ca,%ebx
 8048425:	e8 4e fe ff ff       	call   8048278 <_init>
 804842a:	8d 83 20 ff ff ff    	lea    0xffffff20(%ebx),%eax
 8048430:	8d 93 20 ff ff ff    	lea    0xffffff20(%ebx),%edx
 8048436:	89 45 f0             	mov    %eax,0xfffffff0(%ebp)
 8048439:	29 d0                	sub    %edx,%eax
 804843b:	31 f6                	xor    %esi,%esi
 804843d:	c1 f8 02             	sar    $0x2,%eax
 8048440:	39 c6                	cmp    %eax,%esi
 8048442:	73 16                	jae    804845a <__libc_csu_init+0x4a>
 8048444:	89 d7                	mov    %edx,%edi
 8048446:	89 f6                	mov    %esi,%esi
 8048448:	ff 14 b2             	call   *(%edx,%esi,4)
 804844b:	8b 4d f0             	mov    0xfffffff0(%ebp),%ecx
 804844e:	29 f9                	sub    %edi,%ecx
 8048450:	46                   	inc    %esi
 8048451:	c1 f9 02             	sar    $0x2,%ecx
 8048454:	39 ce                	cmp    %ecx,%esi
 8048456:	89 fa                	mov    %edi,%edx
 8048458:	72 ee                	jb     8048448 <__libc_csu_init+0x38>
 804845a:	83 c4 0c             	add    $0xc,%esp
 804845d:	5b                   	pop    %ebx
 804845e:	5e                   	pop    %esi
 804845f:	5f                   	pop    %edi
 8048460:	c9                   	leave  
 8048461:	c3                   	ret    
 8048462:	89 f6                	mov    %esi,%esi

08048464 <__libc_csu_fini>:
 8048464:	55                   	push   %ebp
 8048465:	89 e5                	mov    %esp,%ebp
 8048467:	57                   	push   %edi
 8048468:	56                   	push   %esi
 8048469:	53                   	push   %ebx
 804846a:	e8 00 00 00 00       	call   804846f <__libc_csu_fini+0xb>
 804846f:	5b                   	pop    %ebx
 8048470:	81 c3 79 11 00 00    	add    $0x1179,%ebx
 8048476:	8d 83 20 ff ff ff    	lea    0xffffff20(%ebx),%eax
 804847c:	8d bb 20 ff ff ff    	lea    0xffffff20(%ebx),%edi
 8048482:	29 f8                	sub    %edi,%eax
 8048484:	c1 f8 02             	sar    $0x2,%eax
 8048487:	83 ec 0c             	sub    $0xc,%esp
 804848a:	8d 70 ff             	lea    0xffffffff(%eax),%esi
 804848d:	eb 05                	jmp    8048494 <__libc_csu_fini+0x30>
 804848f:	90                   	nop    
 8048490:	ff 14 b7             	call   *(%edi,%esi,4)
 8048493:	4e                   	dec    %esi
 8048494:	83 fe ff             	cmp    $0xffffffff,%esi
 8048497:	75 f7                	jne    8048490 <__libc_csu_fini+0x2c>
 8048499:	e8 2e 00 00 00       	call   80484cc <_fini>
 804849e:	83 c4 0c             	add    $0xc,%esp
 80484a1:	5b                   	pop    %ebx
 80484a2:	5e                   	pop    %esi
 80484a3:	5f                   	pop    %edi
 80484a4:	c9                   	leave  
 80484a5:	c3                   	ret    
 80484a6:	90                   	nop    
 80484a7:	90                   	nop    

080484a8 <__do_global_ctors_aux>:
 80484a8:	55                   	push   %ebp
 80484a9:	89 e5                	mov    %esp,%ebp
 80484ab:	53                   	push   %ebx
 80484ac:	52                   	push   %edx
 80484ad:	bb 08 95 04 08       	mov    $0x8049508,%ebx
 80484b2:	a1 08 95 04 08       	mov    0x8049508,%eax
 80484b7:	eb 0a                	jmp    80484c3 <__do_global_ctors_aux+0x1b>
 80484b9:	8d 76 00             	lea    0x0(%esi),%esi
 80484bc:	83 eb 04             	sub    $0x4,%ebx
 80484bf:	ff d0                	call   *%eax
 80484c1:	8b 03                	mov    (%ebx),%eax
 80484c3:	83 f8 ff             	cmp    $0xffffffff,%eax
 80484c6:	75 f4                	jne    80484bc <__do_global_ctors_aux+0x14>
 80484c8:	58                   	pop    %eax
 80484c9:	5b                   	pop    %ebx
 80484ca:	c9                   	leave  
 80484cb:	c3                   	ret    
Disassembly of section .fini:

080484cc <_fini>:
 80484cc:	55                   	push   %ebp
 80484cd:	89 e5                	mov    %esp,%ebp
 80484cf:	53                   	push   %ebx
 80484d0:	e8 00 00 00 00       	call   80484d5 <_fini+0x9>
 80484d5:	5b                   	pop    %ebx
 80484d6:	81 c3 13 11 00 00    	add    $0x1113,%ebx
 80484dc:	50                   	push   %eax
 80484dd:	e8 26 fe ff ff       	call   8048308 <__do_global_dtors_aux>
 80484e2:	59                   	pop    %ecx
 80484e3:	5b                   	pop    %ebx
 80484e4:	c9                   	leave  
 80484e5:	c3                   	ret    
