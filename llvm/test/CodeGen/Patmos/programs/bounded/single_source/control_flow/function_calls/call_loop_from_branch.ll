; RUN: EXEC_ARGS="0=2 1=5 2=4 3=7 4=6"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests calling a function containing a loop from another function's branch path
; 
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1

define i32 @add_to(i32 %x, i32 %iterations)  {
entry:
  br label %loop
  
loop:
  %x.phi = phi i32 [ %x, %entry ], [ %x.add, %loop ]
  %i = phi i32 [ %iterations, %entry ], [ %i.dec, %loop ]
  %cmp = icmp eq i32 %i, 0
  %i.dec = sub nsw i32 %i, 1
  %one = load volatile i32, i32* @_1
  %x.add = add i32 %x.phi, %one
  call void @llvm.loop.bound(i32 0, i32 4)
  br i1 %cmp, label %end, label %loop

end:
  ret i32 %x.add
}

define i32 @main(i32 %x)  {
entry:
  %even = trunc i32 %x to i1
  br i1 %even, label %if.then, label %if.else
  
if.then:
  %res.then = call i32 @add_to(i32 %x, i32 3)
  br label %end

if.else:
  %res.else = add i32 %x, 2
  br label %end
  

end:
  %result = phi i32 [%res.then, %if.then], [%res.else, %if.else]
  ret i32 %result
}

declare void @llvm.loop.bound(i32, i32)
