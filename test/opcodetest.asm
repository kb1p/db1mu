; This program tests that opcode implementation is correct

    .inesprg    1
    .ineschr    1
    .inesmir    1
    .inesmap    0

    .bank   0
    .org    $C000

; symbolic names of memory locations in page zero
        .rsset  $0000
result  .rs     1
test    .rs     1

; This procedure does the test itself. If some opcode produces
; unexpected result, it must put 1 to the byte $0000, or 0 in case of success.
; This routine presumes that conditional jump instructions are operating correct.
Evaluate:
    sei
    ; Comparison operations test
    lda     #15
    cmp     #12
    bmi     .cmp_fail
    beq     .cmp_fail
    bcc     .cmp_fail
    cmp     #100
    bpl     .cmp_fail
    beq     .cmp_fail
    bcs     .cmp_fail
    lda     #-29
    cmp     #100
    bmi     .cmp_fail
    beq     .cmp_fail
    ;bcs     .cmp_fail
    cmp     #-29
    bcc     .cmp_fail
    bne     .cmp_fail
    bmi     .cmp_fail
    jmp     .cmp_succ
.cmp_fail:
    jmp     .fail
.cmp_succ:
    ; Addition test
    ; p + p
    clc
    lda     #10
    adc     #1
    bcs     .adc_fail
    bmi     .adc_fail
    beq     .adc_fail
    bvs     .adc_fail
    ; p + n
    lda     #10
    adc     #$FF
    bcc     .adc_fail
    bmi     .adc_fail
    beq     .adc_fail
    bvs     .adc_fail
    ; p + p -> overflow
    clc
    lda     #$7F
    adc     #1
    bcs     .adc_fail
    bpl     .adc_fail
    beq     .adc_fail
    bvc     .adc_fail
    ; p + -p == 0
    lda     #$FF
    adc     #1
    bne     .adc_fail
    ; n + n -> overflow
    clc
    clv
    lda     #$-1
    adc     #$-128
    bvc     .adc_fail
    jmp     .adc_succ
.adc_fail:
    jmp     .fail
.adc_succ:
    ; subtraction test
    ; p1 - p2, p2 > p1 -> sign
    lda     #10
    sec
    sbc     #11
    bpl     .sbc_fail
    beq     .sbc_fail
    bvs     .sbc_fail
    ; 1 - -1 == 2
    lda     #1
    sec
    sbc     #-1
    bmi     .sbc_fail
    beq     .sbc_fail
    bvs     .sbc_fail
    cmp     #2
    bne     .sbc_fail
    ; carry influence
    lda     #2
    clc
    sbc     #0
    cmp     #1
    bne     .sbc_fail
    ; overflow:
    clv
    lda     #1
    sec
    sbc     #-128
    bvc     .sbc_fail
    clv
    lda     #-90
    sec
    sbc     #90
    bvc     .sbc_fail
    lda     #-29
    sec
    sbc     #100
    bmi     .sbc_fail
    beq     .sbc_fail
    bvc     .sbc_fail
    ;bcs     .sbc_fail
    jmp     .sbc_succ
.sbc_fail:
    jmp     .fail
.sbc_succ:
    ; rol tests
    clc
    lda     #$FF
    rol     A
    bcc     .rol_fail
    beq     .rol_fail
    bpl     .rol_fail
    rol     A
    bcc     .rol_fail
    bpl     .rol_fail
    cmp     #$FD
    bne     .rol_fail
    sec
    rol     A
    bpl     .rol_fail
    cmp     #$FB
    bne     .rol_fail
    sec
    rol     A
    rol     A
    rol     A
    rol     A
    rol     A
    rol     A
    bcs     .rol_fail
    clc
    lda     #1
    rol     A
    bmi     .rol_fail
    rol     A
    rol     A
    rol     A
    rol     A
    rol     A
    rol     A
    bpl     .rol_fail
    rol     A
    bmi     .rol_fail
    bne     .rol_fail
    bcc     .rol_fail
    jmp     .rol_succ
.rol_fail:
    jmp .fail
.rol_succ:
    ; ror tests
    lda     #%01010101
    clc
    ror     A
    bcc     .ror_fail
    bmi     .ror_fail
    beq     .ror_fail
    ror     A
    bcs     .ror_fail
    bpl     .ror_fail
    beq     .ror_fail
    ror     A
    ror     A
    ror     A
    ror     A
    ror     A
    ror     A
    ror     A
    cmp     #%01010101
    bne     .ror_fail
    clc
    lda     #1
    ror     A
    bne     .ror_fail
    bcc     .ror_fail
    bmi     .ror_fail
    ror     A
    beq     .ror_fail
    bcs     .ror_fail
    bpl     .ror_fail
    jmp     .ror_succ
.ror_fail:
    jmp     .fail
.ror_succ:
    ; Result saving: 0 means success (default), 1 failure
    lda     #0
    jmp     .saveResult
.fail:
    lda     #1
.saveResult:
    sta     <result
; wait for vblank
.wait:
    lda     $2002
    bpl     .wait
    lda     #$80
    sta     $2000
    lda     #$0B
    sta     $2001
.loop:
    jmp     .loop

Render:
    ; PPU state check can be skipped for db1mu (temporarily)
    ; Set address in VRAM pointing to background color
    lda     #$3F
    sta     $2006
    lda     #$00
    sta     $2006

    ; test result
    lda     <result
    bne     .red
    lda     #%00001100
    sta     $2007
    jmp     .quit
.red:
    lda     #%00110000
    sta     $2007
.quit:
    ; disable sprites / background tiles, render only solid color
    lda     #0
    sta     $2001
    rti

Dummy:
    rti

    .bank   1
    .org    $FFFA
    .dw     Render
    .dw     Evaluate
    .dw     Dummy

    .bank 2
    .org  $0000
    .incbin "opcodetest.chr"
