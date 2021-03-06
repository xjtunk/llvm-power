; RUN: llvm-as < %s | opt -tailcallelim | llvm-dis | \
; RUN:    %prcontext alloca 1 | grep {i32 @foo}

declare void @bar(i32*)

define i32 @foo() {
	%A = alloca i32		; <i32*> [#uses=2]
	store i32 17, i32* %A
	call void @bar( i32* %A )
	%X = tail call i32 @foo( )		; <i32> [#uses=1]
	ret i32 %X
}

