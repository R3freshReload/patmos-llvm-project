; RUN: llc < %s | FileCheck %s
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests that loop bounds are output as a comment following the loop header block's own comments
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

; CHECK-LABEL: main:
define i32 @main(i32 %iteration_count)  {
entry:
  br label %for.cond

; CHECK-LABEL: 		# %for.cond
; CHECK: 	# =>This Inner Loop Header: Depth=1
; CHECK: 	# Loop bound: [1, 100]
for.cond:                                         ; preds = %for.inc, %entry
  %x.0 = phi i32 [ 0, %entry ], [ %add, %for.body ]
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.body ]
  %cmp = icmp slt i32 %i.0, %iteration_count
  call void @llvm.loop.bound(i32 0, i32 99)
  br i1 %cmp, label %for.body, label %for.end

; CHECK-LABEL: 		# %for.body
for.body:                                          ; preds = %for.body
  %shift = shl nsw i32 %i.0, 1
  %add = add nsw i32 %x.0, %shift
  %inc = add nsw i32 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret i32 %x.0
}

declare void @llvm.loop.bound(i32, i32)

