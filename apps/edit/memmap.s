; memmap.s -- limites de la RAM libre, fixees a l'edition de liens : le tampon
; de texte va de la fin du BSS au bas de la pile.

        .import __BSS_RUN__, __BSS_SIZE__, __HIMEM__, __STACKSIZE__
        .export _tb_mem_start, _tb_mem_end

        .rodata
_tb_mem_start:  .word __BSS_RUN__ + __BSS_SIZE__
_tb_mem_end:    .word __HIMEM__ - __STACKSIZE__
