; screen_a.s -- boucles internes du rendu, en assembleur 6502 (cc65 seulement ;
; la version hote (PC) utilise les equivalents en C de screen.c).
;
; Les parametres passent par des variables globales de screen.c : evite la pile
; logicielle de cc65, qui coute plus cher que ces boucles de quelques octets.

        .importzp ptr1, ptr2
        .import   _run_dst, _run_src, _run_n, _run_code, _run_aux
        .export   _put_run, _fill_run, _gather_run

; u8 put_run(void)  -- ecrit run_n cellules a run_dst depuis la chaine run_src :
; ASCII imprimable -> code ecran normal ($A0-$FE) ; apres un NUL, ou pour tout
; caractere hors $20-$7E, on ecrit un espace. Renvoie 1 si une cellule a change.
_put_run:
        lda _run_dst
        sta ptr1
        lda _run_dst+1
        sta ptr1+1
        lda _run_src
        sta ptr2
        lda _run_src+1
        sta ptr2+1
        ldx #0                  ; X = drapeau "modifie"
        ldy #0
@loop:  cpy _run_n
        beq @done
        lda (ptr2),y
        beq @pad                ; NUL : fin de chaine
        cmp #$20
        bcc @space
        cmp #$7F
        bcs @space
        ora #$80
@store: cmp (ptr1),y
        beq @same
        sta (ptr1),y
        ldx #1
@same:  iny
        bne @loop
@done:  txa
        ldx #0
        rts
@space: lda #$A0
        bne @store
@pad:   lda #$A0                ; reste de la zone : espaces
@padlp: cpy _run_n
        beq @done
        cmp (ptr1),y
        beq @psame
        sta (ptr1),y
        ldx #1
@psame: iny
        bne @padlp
        beq @done

; u8 fill_run(void) -- run_n cellules a run_dst := run_code. Renvoie 1 si modifie.
_fill_run:
        lda _run_dst
        sta ptr1
        lda _run_dst+1
        sta ptr1+1
        lda _run_code
        ldx #0
        ldy #0
@floop: cpy _run_n
        beq @fdone
        cmp (ptr1),y
        beq @fsame
        sta (ptr1),y
        ldx #1
@fsame: iny
        bne @floop
@fdone: txa
        ldx #0
        rts

; void gather_run(void) -- run_n octets : run_dst[i] = run_src[2*i]
; (colonnes paires ou impaires d'une ligne, vers la memoire video).
; Si run_aux != 0, la copie se fait vers la memoire AUXILIAIRE : PAGE2 n'est actif
; que pendant la boucle, interruptions masquees -- le firmware de la souris
; (interruption VBL) utilise les "trous" de la page texte et se tromperait de
; banque si PAGE2 restait actif entre deux lignes.
; Code auto-modifie (le programme est en RAM) : lda/sta absolus indexes, sans
; pointeur a incrementer -- 24 cycles par cellule.
_gather_run:
        lda _run_src
        sta @rd+1
        lda _run_src+1
        sta @rd+2
        lda _run_dst
        sta @wr+1
        lda _run_dst+1
        sta @wr+2
        php
        sei
        lda _run_aux
        beq @main
        sta $C055               ; PAGE2 on (80STORE actif : la page texte devient l'aux)
@main:  ldx #0                  ; X = indice destination
        ldy #0                  ; Y = indice source (pas de 2 ; run_n <= 127)
@gloop: cpx _run_n
        beq @gdone
@rd:    lda $FFFF,y
@wr:    sta $FFFF,x
        inx
        iny
        iny
        bne @gloop
@gdone: sta $C054               ; PAGE2 off (ecriture sans effet de bord sur A)
        plp
        rts
