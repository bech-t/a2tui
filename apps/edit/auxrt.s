; auxrt.s -- copies entre la RAM principale et la RAM auxiliaire.
;
; Le noyau de copie (aux_rt .. aux_rt_end) est installe aux adresses $0300 des
; DEUX banques : RAMRD/RAMWR changent aussi la banque d'ou le processeur lit
; ses instructions, donc le meme code doit se trouver au meme endroit dans les
; deux. Il ne contient que des branchements relatifs.
;
; Entree de aux_rt : A = mode (bit 0 : lire dans l'aux ; bit 1 : ecrire dans
; l'aux ; bit 2 : copier de la fin vers le debut, pour un chevauchement vers
; le haut), ptr1 = source, ptr2 = destination, ptr3 = nombre d'octets.

        .importzp ptr1, ptr2, ptr3, tmp1
        .export _aux_run, _aux_rt, _aux_rt_end
        .export _aux_src, _aux_dst, _aux_cnt

RAMRDOFF = $C002
RAMRDON  = $C003
RAMWROFF = $C004
RAMWRON  = $C005
AUXRT    = $0300

        .bss
_aux_src: .res 2
_aux_dst: .res 2
_aux_cnt: .res 2

        .code

; void __fastcall__ aux_run(unsigned char mode) : copie decrite par
; aux_src / aux_dst / aux_cnt. Interruptions coupees pendant la copie.
_aux_run:
        sta tmp1
        lda _aux_src
        sta ptr1
        lda _aux_src+1
        sta ptr1+1
        lda _aux_dst
        sta ptr2
        lda _aux_dst+1
        sta ptr2+1
        lda _aux_cnt
        sta ptr3
        lda _aux_cnt+1
        sta ptr3+1
        lda tmp1
        php
        sei
        jsr AUXRT
        plp
        rts

_aux_rt:
        tax
        and #1
        beq @w
        sta RAMRDON
@w:     txa
        and #2
        beq @d
        sta RAMWRON
@d:     txa
        and #4
        bne @bwd

        ldy #0                  ; vers l'avant : pages entieres, puis le reste
        lda ptr3+1
        beq @frem
@fpg:   lda (ptr1),y
        sta (ptr2),y
        iny
        bne @fpg
        inc ptr1+1
        inc ptr2+1
        dec ptr3+1
        bne @fpg
@frem:  cpy ptr3
        beq @done
@fb:    lda (ptr1),y
        sta (ptr2),y
        iny
        cpy ptr3
        bne @fb
        clc
        bcc @done

@bwd:   lda ptr1+1              ; vers l'arriere : on part du dernier octet
        clc
        adc ptr3+1
        sta ptr1+1
        lda ptr2+1
        clc
        adc ptr3+1
        sta ptr2+1
        ldy ptr3
        beq @bpages
@br:    dey
        lda (ptr1),y
        sta (ptr2),y
        tya
        bne @br
@bpages:
        lda ptr3+1
        beq @done
@bpg:   dec ptr1+1
        dec ptr2+1
        ldy #0
@bp:    dey
        lda (ptr1),y
        sta (ptr2),y
        tya
        bne @bp
        dec ptr3+1
        bne @bpg

@done:  sta RAMRDOFF
        sta RAMWROFF
        rts
_aux_rt_end:
