; RUN: EXEC_ARGS="0=0 1=1 2=14 9=45 10=230"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests the code generated for two loops, each in a branch. 
; 
; The following is the equivalent C code:
; 
; volatile int _1 = 1;
; volatile int _2 = 2;
; 
; int main(int cond){
; 	int x = 0;
; 	if(cond%2){
; 		#pragma loopbound min 0 max 10
; 		for(int i = 0; i<cond; i++){
; 			x += i + _1;
; 		}
; 	}else{
; 		#pragma loopbound min 0 max 20
; 		for(int i = 0; i<(2*cond); i++){
; 			x += i + _2;
; 		}
; 	}
; 	return x;
; }
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1
@_2 = global i32 2

define i32 @main(i32 %cond) {
entry:
  %0 = alloca i32
  %x = alloca i32
  %i = alloca i32
  %i1 = alloca i32
  store i32 %cond, i32* %0
  store i32 0, i32* %x
  %1 = load i32, i32* %0
  %2 = srem i32 %1, 2
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  store i32 0, i32* %i
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %if.then
  %4 = load i32, i32* %i
  %5 = load i32, i32* %0
  %6 = icmp slt i32 %4, %5
  call void @llvm.loop.bound(i32 0, i32 10)
  br i1 %6, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %7 = load i32, i32* %i
  %8 = load volatile i32, i32* @_1
  %9 = add nsw i32 %7, %8
  %10 = load i32, i32* %x
  %11 = add nsw i32 %10, %9
  store i32 %11, i32* %x
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %12 = load i32, i32* %i
  %13 = add nsw i32 %12, 1
  store i32 %13, i32* %i
  br label %for.cond

for.end:                                          ; preds = %for.cond
  br label %if.end

if.else:                                          ; preds = %entry
  store i32 0, i32* %i1
  br label %for.cond2

for.cond2:                                        ; preds = %for.inc4, %if.else
  %14 = load i32, i32* %i1
  %15 = load i32, i32* %0
  %16 = mul nsw i32 2, %15
  %17 = icmp slt i32 %14, %16
  call void @llvm.loop.bound(i32 0, i32 20)
  br i1 %17, label %for.body3, label %for.end5

for.body3:                                        ; preds = %for.cond2
  %18 = load i32, i32* %i1
  %19 = load volatile i32, i32* @_2
  %20 = add nsw i32 %18, %19
  %21 = load i32, i32* %x
  %22 = add nsw i32 %21, %20
  store i32 %22, i32* %x
  br label %for.inc4

for.inc4:                                         ; preds = %for.body3
  %23 = load i32, i32* %i1
  %24 = add nsw i32 %23, 1
  store i32 %24, i32* %i1
  br label %for.cond2

for.end5:                                         ; preds = %for.cond2
  br label %if.end

if.end:                                           ; preds = %for.end5, %for.end
  %25 = load i32, i32* %x
  ret i32 %25
}

declare void @llvm.loop.bound(i32, i32)
