; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -indvars -S < %s | FileCheck %s
; RUN: opt -passes=indvars -S < %s | FileCheck %s

; TODO: should be able to remove the range check basing on the following facts:
; 0 <= len <= MAX_INT [1];
; iv starts from len and goes down stopping at zero and [1], therefore
;   0 <= iv <= len [2];
; 3. In range_check_block, iv != 0 and [2], therefore
;   1 <= iv <= len [3];
; 4. iv.next = iv - 1 and [3], therefore
;   0 <= iv.next < len.
define void @test_predicated_simple(i32* %p, i32* %arr) {
; CHECK-LABEL: @test_predicated_simple(
; CHECK-NEXT:  preheader:
; CHECK-NEXT:    [[LEN:%.*]] = load i32, i32* [[P:%.*]], align 4, [[RNG0:!range !.*]]
; CHECK-NEXT:    br label [[LOOP:%.*]]
; CHECK:       loop:
; CHECK-NEXT:    [[IV:%.*]] = phi i32 [ [[LEN]], [[PREHEADER:%.*]] ], [ [[IV_NEXT:%.*]], [[BACKEDGE:%.*]] ]
; CHECK-NEXT:    [[ZERO_COND:%.*]] = icmp eq i32 [[IV]], 0
; CHECK-NEXT:    br i1 [[ZERO_COND]], label [[EXIT:%.*]], label [[RANGE_CHECK_BLOCK:%.*]]
; CHECK:       range_check_block:
; CHECK-NEXT:    [[IV_NEXT]] = sub i32 [[IV]], 1
; CHECK-NEXT:    [[RANGE_CHECK:%.*]] = icmp ult i32 [[IV_NEXT]], [[LEN]]
; CHECK-NEXT:    br i1 [[RANGE_CHECK]], label [[BACKEDGE]], label [[FAIL:%.*]]
; CHECK:       backedge:
; CHECK-NEXT:    [[EL_PTR:%.*]] = getelementptr i32, i32* [[P]], i32 [[IV]]
; CHECK-NEXT:    [[EL:%.*]] = load i32, i32* [[EL_PTR]], align 4
; CHECK-NEXT:    [[LOOP_COND:%.*]] = icmp eq i32 [[EL]], 0
; CHECK-NEXT:    br i1 [[LOOP_COND]], label [[LOOP]], label [[EXIT]]
; CHECK:       exit:
; CHECK-NEXT:    ret void
; CHECK:       fail:
; CHECK-NEXT:    unreachable
;
preheader:
  %len = load i32, i32* %p, !range !0
  br label %loop

loop:
  %iv = phi i32 [%len, %preheader], [%iv.next, %backedge]
  %zero_cond = icmp eq i32 %iv, 0
  br i1 %zero_cond, label %exit, label %range_check_block

range_check_block:
  %iv.next = sub i32 %iv, 1
  %range_check = icmp ult i32 %iv.next, %len
  br i1 %range_check, label %backedge, label %fail

backedge:
  %el.ptr = getelementptr i32, i32* %p, i32 %iv
  %el = load i32, i32* %el.ptr
  %loop.cond = icmp eq i32 %el, 0
  br i1 %loop.cond, label %loop, label %exit

exit:
  ret void

fail:
  unreachable
}

!0 = !{i32 0, i32 2147483647}
