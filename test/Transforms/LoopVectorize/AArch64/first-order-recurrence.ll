; RUN: opt < %s -loop-vectorize -force-vector-width=4 -force-vector-interleave=1 -dce -instcombine -S | FileCheck %s
; RUN: opt < %s -loop-vectorize -force-vector-width=4 -force-vector-interleave=2 -dce -instcombine -S | FileCheck %s --check-prefix=UNROLL

target datalayout = "e-m:e-i64:64-i128:128-n32:64-S128"

; CHECK-LABEL: @recurrence_1
;
; void recurrence_1(int *a, int *b, int n) {
;   for(int i = 0; i < n; i++)
;     b[i] =  a[i] + a[i - 1]
; }
;
; CHECK:  vector.ph:
; CHECK:    %vector.recur.init = insertelement <4 x i32> undef, i32 %pre_load, i32 3
;
; CHECK:  vector.body:
; CHECK:    %vector.recur = phi <4 x i32> [ %vector.recur.init, %vector.ph ], [ [[L1:%[a-zA-Z0-9.]+]], %vector.body ]
; CHECK:    [[L1]] = load <4 x i32>
; CHECK:    {{.*}} = shufflevector <4 x i32> %vector.recur, <4 x i32> [[L1]], <4 x i32> <i32 3, i32 4, i32 5, i32 6>
;
; CHECK:  middle.block:
; CHECK:    %vector.recur.extract = extractelement <4 x i32> [[L1]], i32 3
;
; CHECK:  scalar.ph:
; CHECK:    %scalar.recur.init = phi i32 [ %vector.recur.extract, %middle.block ], [ %pre_load, %vector.memcheck ], [ %pre_load, %min.iters.checked ], [ %pre_load, %for.preheader ]
;
; CHECK:  scalar.body:
; CHECK:    %scalar.recur = phi i32 [ %scalar.recur.init, %scalar.ph ], [ {{.*}}, %scalar.body ]
;
; UNROLL: vector.body:
; UNROLL:   %vector.recur = phi <4 x i32> [ %vector.recur.init, %vector.ph ], [ [[L2:%[a-zA-Z0-9.]+]], %vector.body ]
; UNROLL:   [[L1:%[a-zA-Z0-9.]+]] = load <4 x i32>
; UNROLL:   [[L2]] = load <4 x i32>
; UNROLL:   {{.*}} = shufflevector <4 x i32> %vector.recur, <4 x i32> [[L1]], <4 x i32> <i32 3, i32 4, i32 5, i32 6>
; UNROLL:   {{.*}} = shufflevector <4 x i32> [[L1]], <4 x i32> [[L2]], <4 x i32> <i32 3, i32 4, i32 5, i32 6>
;
; UNROLL: middle.block:
; UNROLL:   %vector.recur.extract = extractelement <4 x i32> [[L2]], i32 3
;
define void @recurrence_1(i32* nocapture readonly %a, i32* nocapture %b, i32 %n) {
entry:
  br label %for.preheader

for.preheader:
  %arrayidx.phi.trans.insert = getelementptr inbounds i32, i32* %a, i64 0
  %pre_load = load i32, i32* %arrayidx.phi.trans.insert
  br label %scalar.body

scalar.body:
  %0 = phi i32 [ %pre_load, %for.preheader ], [ %1, %scalar.body ]
  %indvars.iv = phi i64 [ 0, %for.preheader ], [ %indvars.iv.next, %scalar.body ]
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %arrayidx32 = getelementptr inbounds i32, i32* %a, i64 %indvars.iv.next
  %1 = load i32, i32* %arrayidx32
  %arrayidx34 = getelementptr inbounds i32, i32* %b, i64 %indvars.iv
  %add35 = add i32 %1, %0
  store i32 %add35, i32* %arrayidx34
  %lftr.wideiv = trunc i64 %indvars.iv.next to i32
  %exitcond = icmp eq i32 %lftr.wideiv, %n
  br i1 %exitcond, label %for.exit, label %scalar.body

for.exit:
  ret void
}

; CHECK-LABEL: @recurrence_2
;
; int recurrence_2(int *a, int n) {
;   int minmax;
;   for (int i = 0; i < n; ++i)
;     minmax = min(minmax, max(a[i] - a[i-1], 0));
;   return minmax;
; }
;
; CHECK:  vector.ph:
; CHECK:    %vector.recur.init = insertelement <4 x i32> undef, i32 %.pre, i32 3
;
; CHECK:  vector.body:
; CHECK:    %vector.recur = phi <4 x i32> [ %vector.recur.init, %vector.ph ], [ [[L1:%[a-zA-Z0-9.]+]], %vector.body ]
; CHECK:    [[L1]] = load <4 x i32>
; CHECK:    {{.*}} = shufflevector <4 x i32> %vector.recur, <4 x i32> [[L1]], <4 x i32> <i32 3, i32 4, i32 5, i32 6>
;
; CHECK:  middle.block:
; CHECK:    %vector.recur.extract = extractelement <4 x i32> [[L1]], i32 3
;
; CHECK:  scalar.ph:
; CHECK:    %scalar.recur.init = phi i32 [ %vector.recur.extract, %middle.block ], [ %.pre, %min.iters.checked ], [ %.pre, %for.preheader ]
;
; CHECK:  scalar.body:
; CHECK:    %scalar.recur = phi i32 [ %scalar.recur.init, %scalar.ph ], [ {{.*}}, %scalar.body ]
;
; UNROLL: vector.body:
; UNROLL:   %vector.recur = phi <4 x i32> [ %vector.recur.init, %vector.ph ], [ [[L2:%[a-zA-Z0-9.]+]], %vector.body ]
; UNROLL:   [[L1:%[a-zA-Z0-9.]+]] = load <4 x i32>
; UNROLL:   [[L2]] = load <4 x i32>
; UNROLL:   {{.*}} = shufflevector <4 x i32> %vector.recur, <4 x i32> [[L1]], <4 x i32> <i32 3, i32 4, i32 5, i32 6>
; UNROLL:   {{.*}} = shufflevector <4 x i32> [[L1]], <4 x i32> [[L2]], <4 x i32> <i32 3, i32 4, i32 5, i32 6>
;
; UNROLL: middle.block:
; UNROLL:   %vector.recur.extract = extractelement <4 x i32> [[L2]], i32 3
;
define i32 @recurrence_2(i32* nocapture readonly %a, i32 %n) {
entry:
  %cmp27 = icmp sgt i32 %n, 0
  br i1 %cmp27, label %for.preheader, label %for.cond.cleanup

for.preheader:
  %arrayidx2.phi.trans.insert = getelementptr inbounds i32, i32* %a, i64 -1
  %.pre = load i32, i32* %arrayidx2.phi.trans.insert, align 4
  br label %scalar.body

for.cond.cleanup.loopexit:
  %minmax.0.cond.lcssa = phi i32 [ %minmax.0.cond, %scalar.body ]
  br label %for.cond.cleanup

for.cond.cleanup:
  %minmax.0.lcssa = phi i32 [ undef, %entry ], [ %minmax.0.cond.lcssa, %for.cond.cleanup.loopexit ]
  ret i32 %minmax.0.lcssa

scalar.body:
  %0 = phi i32 [ %.pre, %for.preheader ], [ %1, %scalar.body ]
  %indvars.iv = phi i64 [ 0, %for.preheader ], [ %indvars.iv.next, %scalar.body ]
  %minmax.028 = phi i32 [ undef, %for.preheader ], [ %minmax.0.cond, %scalar.body ]
  %arrayidx = getelementptr inbounds i32, i32* %a, i64 %indvars.iv
  %1 = load i32, i32* %arrayidx, align 4
  %sub3 = sub nsw i32 %1, %0
  %cmp4 = icmp sgt i32 %sub3, 0
  %cond = select i1 %cmp4, i32 %sub3, i32 0
  %cmp5 = icmp slt i32 %minmax.028, %cond
  %minmax.0.cond = select i1 %cmp5, i32 %minmax.028, i32 %cond
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %lftr.wideiv = trunc i64 %indvars.iv.next to i32
  %exitcond = icmp eq i32 %lftr.wideiv, %n
  br i1 %exitcond, label %for.cond.cleanup.loopexit, label %scalar.body
}

; CHECK-LABEL: @recurrence_3
;
; void recurrence_3(short *a, double *b, int n, float f, short p) {
;   b[0] = (double)a[0] - f * (double)p;
;   for (int i = 1; i < n; i++)
;     b[i] = (double)a[i] - f * (double)a[i - 1];
; }
;
;
; CHECK:  vector.ph:
; CHECK:    %vector.recur.init = insertelement <4 x i16> undef, i16 %0, i32 3
;
; CHECK:  vector.body:
; CHECK:    %vector.recur = phi <4 x i16> [ %vector.recur.init, %vector.ph ], [ [[L1:%[a-zA-Z0-9.]+]], %vector.body ]
; CHECK:    [[L1]] = load <4 x i16>
; CHECK:    {{.*}} = shufflevector <4 x i16> %vector.recur, <4 x i16> [[L1]], <4 x i32> <i32 3, i32 4, i32 5, i32 6>
;
; CHECK:  middle.block:
; CHECK:    %vector.recur.extract = extractelement <4 x i16> [[L1]], i32 3
;
; CHECK:  scalar.ph:
; CHECK:    %scalar.recur.init = phi i16 [ %vector.recur.extract, %middle.block ], [ %0, %vector.memcheck ], [ %0, %min.iters.checked ], [ %0, %for.preheader ]
;
; CHECK:  scalar.body:
; CHECK:    %scalar.recur = phi i16 [ %scalar.recur.init, %scalar.ph ], [ {{.*}}, %scalar.body ]
;
; UNROLL: vector.body:
; UNROLL:   %vector.recur = phi <4 x i16> [ %vector.recur.init, %vector.ph ], [ [[L2:%[a-zA-Z0-9.]+]], %vector.body ]
; UNROLL:   [[L1:%[a-zA-Z0-9.]+]] = load <4 x i16>
; UNROLL:   [[L2]] = load <4 x i16>
; UNROLL:   {{.*}} = shufflevector <4 x i16> %vector.recur, <4 x i16> [[L1]], <4 x i32> <i32 3, i32 4, i32 5, i32 6>
; UNROLL:   {{.*}} = shufflevector <4 x i16> [[L1]], <4 x i16> [[L2]], <4 x i32> <i32 3, i32 4, i32 5, i32 6>
;
; UNROLL: middle.block:
; UNROLL:   %vector.recur.extract = extractelement <4 x i16> [[L2]], i32 3
;
define void @recurrence_3(i16* nocapture readonly %a, double* nocapture %b, i32 %n, float %f, i16 %p) {
entry:
  %0 = load i16, i16* %a, align 2
  %conv = sitofp i16 %0 to double
  %conv1 = fpext float %f to double
  %conv2 = sitofp i16 %p to double
  %mul = fmul fast double %conv2, %conv1
  %sub = fsub fast double %conv, %mul
  store double %sub, double* %b, align 8
  %cmp25 = icmp sgt i32 %n, 1
  br i1 %cmp25, label %for.preheader, label %for.end

for.preheader:
  br label %scalar.body

scalar.body:
  %1 = phi i16 [ %0, %for.preheader ], [ %2, %scalar.body ]
  %advars.iv = phi i64 [ %advars.iv.next, %scalar.body ], [ 1, %for.preheader ]
  %arrayidx5 = getelementptr inbounds i16, i16* %a, i64 %advars.iv
  %2 = load i16, i16* %arrayidx5, align 2
  %conv6 = sitofp i16 %2 to double
  %conv11 = sitofp i16 %1 to double
  %mul12 = fmul fast double %conv11, %conv1
  %sub13 = fsub fast double %conv6, %mul12
  %arrayidx15 = getelementptr inbounds double, double* %b, i64 %advars.iv
  store double %sub13, double* %arrayidx15, align 8
  %advars.iv.next = add nuw nsw i64 %advars.iv, 1
  %lftr.wideiv = trunc i64 %advars.iv.next to i32
  %exitcond = icmp eq i32 %lftr.wideiv, %n
  br i1 %exitcond, label %for.end.loopexit, label %scalar.body

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}
