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
    ; ora test
    lda     #0
    ora     #$FF
    bpl     .ora_fail
    beq     .ora_fail
    cmp     #$FF
    bne     .ora_fail
    lda     #0
    ora     #0
    bne     .ora_fail
    bmi     .ora_fail
    ora     #1
    beq     .ora_fail
    bmi     .ora_fail
    ora     #$80
    beq     .ora_fail
    bpl     .ora_fail
    jmp     .ora_succ
.ora_fail:
    jmp     .fail
.ora_succ:
    ; bit test
    lda     #$FF
    sta     <test
    lda     #10
    bit     <test
    beq     .bit_fail
    bpl     .bit_fail
    bvc     .bit_fail
    lda     #0
    bit     <test
    bne     .bit_fail
    bpl     .bit_fail
    bvc     .bit_fail
    lda     #12
    sta     <test
    lda     #1
    bit     <test
    bne     .bit_fail
    bmi     .bit_fail
    bvs     .bit_fail
    jmp     .bit_succ
.bit_fail:
    jmp     .fail
.bit_succ:
    ; and operation test
    lda     #$FF
    and     #123
    beq     .and_fail
    bmi     .and_fail
    cmp     #123
    bne     .and_fail
    lda     #$e0
    and     #$c0
    beq     .and_fail
    bpl     .and_fail
    cmp     #$c0
    bne     .and_fail
    lda     #-128
    and     #127
    bne     .and_fail
    bmi     .and_fail
    lda     #0
    and     #$FF
    bne     .and_fail
    bmi     .and_fail
    jmp     .and_succ
.and_fail:
    jmp     .fail
.and_succ:
    ; inc operation test
    lda     #0
    sta     <test
    inc     <test
    beq     .inc_fail
    bmi     .inc_fail
    lda     #1
    cmp     <test
    bne     .inc_fail
    inc     <test
    lda     #2
    cmp     <test
    bne     .inc_fail
    lda     #$FE
    sta     <test
    inc     <test
    beq     .inc_fail
    bpl     .inc_fail
    inc     <test
    bne     .inc_fail
    bmi     .inc_fail
    lda     #$7E
    sta     <test
    inc     <test
    bmi     .inc_fail
    inc     <test
    bpl     .inc_fail
    jmp     .inc_succ
.inc_fail:
    jmp     .fail
.inc_succ:
    ; inx operation test
    ldx     #0
    inx
    beq     .inx_fail
    bmi     .inx_fail
    ldx     #1
    cpx     #1
    bne     .inx_fail
    inx
    cpx     #2
    bne     .inx_fail
    ldx     #$FE
    inx
    beq     .inx_fail
    bpl     .inx_fail
    inx
    bne     .inx_fail
    bmi     .inx_fail
    ldx     #$7E
    inx
    bmi     .inx_fail
    inx
    bpl     .inx_fail
    jmp     .inx_succ
.inx_fail:
    jmp     .fail
.inx_succ:
    ; dec test
    lda     #2
    sta     <test
    dec     <test
    beq     .dec_fail
    bmi     .dec_fail
    dec     <test
    bne     .dec_fail
    bmi     .dec_fail
    dec     <test
    beq     .dec_fail
    bpl     .dec_fail
    ldy     #$FF
    cpy     <test
    bne     .dec_fail
    dec     <test
    bpl     .dec_fail
    beq     .dec_fail
    ldx     #$81
    stx     <test
    dec     <test
    bpl     .dec_fail
    beq     .dec_fail
    dec     <test
    bmi     .dec_fail
    beq     .dec_fail
    ldx     #$7F
    cpx     <test
    bne     .dec_fail
    jmp     .dec_succ
.dec_fail:
    jmp     .fail
.dec_succ:
    ; dey test
    ldy     #2
    dey
    beq     .dey_fail
    bmi     .dey_fail
    dey
    bne     .dey_fail
    bmi     .dey_fail
    dey
    beq     .dey_fail
    bpl     .dey_fail
    cpy     #$FF
    bne     .dey_fail
    dey
    bpl     .dey_fail
    beq     .dey_fail
    ldy     #$81
    dey
    bpl     .dey_fail
    beq     .dey_fail
    dey
    bmi     .dey_fail
    beq     .dey_fail
    cpy     #$7F
    bne     .dey_fail
    jmp     .dey_succ
.dey_fail:
    jmp     .fail
.dey_succ:
    ; eor test
    lda     #0
    bne     .eor_fail
    eor     #$FF
    bpl     .eor_fail
    beq     .eor_fail
    cmp     #$FF
    bne     .eor_fail
    eor     #%10101010
    bmi     .eor_fail
    beq     .eor_fail
    eor     #%01010101
    bmi     .eor_fail
    bne     .eor_fail
    lda     #$FF
    bpl     .eor_fail
    eor     #$7F
    bpl     .eor_fail
    beq     .eor_fail
    eor     #$80
    bne     .eor_fail
    bmi     .eor_fail
    jmp     .eor_succ
.eor_fail:
    jmp     .fail
.eor_succ:
    ; Result saving: green color means success (default), red - failure
    lda     #%00001100
    jmp     .saveResult
.fail:
    lda     #%00110000
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
    sta     $2007
    ; disable sprites / background tiles, render only solid color
    lda     #0
    sta     $2001
    rti

Interrupt:
    lda     #%00000011
    sta     <result
    rti

    .bank   1
    .org    $FFFA
    .dw     Render
    .dw     Evaluate
    .dw     Interrupt

    .bank 2
    .org  $0000
    .incbin "opcodetest.chr"
