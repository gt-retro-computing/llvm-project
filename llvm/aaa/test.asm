	.text
	.text
	;.globl	_fibonacci                      ; -- Begin function fibonacci
_fibonacci:                             ; @fibonacci
; %bb.0:                                ; %entry
	push	ix
	ld	ix, 0
	add	ix, sp
	push	hl
	ld	e, (ix+4)
	ld	d, (ix+5)
	ld	l, e
	ld	h, d
	add	hl, bc
	or	a, a
	sbc	hl, bc
	jp	nz, BB0_1
; %bb.7:
	push	de
	pop	iy
	jp	BB0_5
BB0_1:                                  ; %entry
	ld	iy, 1
	ld	bc, 1
	ld	l, e
	ld	h, d
	or	a, a
	sbc	hl, bc
	jp	nz, BB0_2
; %bb.6:
	jp	BB0_5
BB0_2:                                  ; %for.cond.preheader
	ld	bc, -32768
	ld	l, e
	ld	h, d
	add	hl, bc
	ld	bc, -32766
	or	a, a
	sbc	hl, bc
	jp	c, BB0_5
	jq	c, BB0_5
; %bb.3:                                ; %for.body.preheader
	ld	bc, 0
	dec	de
BB0_4:                                  ; %for.body
                                        ; =>This Inner Loop Header: Depth=1
	push	iy
	ex	(sp), hl
	ld	(ix-2), l
	ld	(ix-1), h
	pop	hl
	add	iy, bc
	dec	de
	ld	l, e
	ld	h, d
	add	hl, bc
	or	a, a
	sbc	hl, bc
	ld	c, (ix-2)
	ld	b, (ix-1)
	jp	nz, BB0_4
BB0_5:                                  ; %cleanup
	push	iy
	pop	hl
	ld	sp, ix
	pop	ix
	ret
	.text