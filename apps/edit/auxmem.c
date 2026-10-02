/* auxmem.c -- RAM auxiliaire (voir auxmem.h). */

#include <string.h>
#include "auxmem.h"

#ifdef TUI_HOST

static u8 host_aux[AUX_CAP];
u8 aux_host_enable;

u8 aux_init(void)
{
    return aux_host_enable;
}

void aux_read(u8 *dst, u16 off, u16 n)
{
    memcpy(dst, host_aux + off, n);
}

void aux_write(u16 off, const u8 *src, u16 n)
{
    memcpy(host_aux + off, src, n);
}

void aux_move(u16 dst, u16 src, u16 n)
{
    memmove(host_aux + dst, host_aux + src, n);
}

#else

#include <fcntl.h>
#include <unistd.h>

#define AUX_BASE    0x0800u
#define AUX_RT      0x0300u
#define AUX_PROBE   0x0380u     /* octet de page 3 : libre dans les deux banques */

#define M_READ_AUX  1
#define M_WRITE_AUX 2
#define M_BACKWARD  4

#define DEVCNT      (*(volatile u8 *)0xBF31)   /* nombre de peripheriques - 1 */
#define DEVLST      ((volatile u8 *)0xBF32)
#define RAMDISK     0xBF                       /* numero d'unite de /RAM */
#define MACHID      (*(volatile u8 *)0xBF98)   /* ProDOS : bits 5-4 = 11 pour 128 Ko */

extern u16 aux_src, aux_dst, aux_cnt;
extern const u8 aux_rt[], aux_rt_end[];
void __fastcall__ aux_run(u8 mode);

static void xfer(u8 mode, u16 src, u16 dst, u16 n)
{
    aux_src = src;
    aux_dst = dst;
    aux_cnt = n;
    aux_run(mode);
}

void aux_read(u8 *dst, u16 off, u16 n)
{
    xfer(M_READ_AUX, (u16)(AUX_BASE + off), (u16)dst, n);
}

void aux_write(u16 off, const u8 *src, u16 n)
{
    xfer(M_WRITE_AUX, (u16)src, (u16)(AUX_BASE + off), n);
}

void aux_move(u16 dst, u16 src, u16 n)
{
    xfer((u8)(M_READ_AUX | M_WRITE_AUX | (dst > src ? M_BACKWARD : 0)),
         (u16)(AUX_BASE + src), (u16)(AUX_BASE + dst), n);
}

/* La RAM auxiliaire repond-elle (verification apres coup : ne s'appelle que
 * si ProDOS en a annonce une, car le noyau de copie lit ses instructions dans
 * la banque auxiliaire pendant une lecture) ? Une ecriture en aux ne doit pas
 * toucher la principale, et se relire en aux. */
static u8 probe(void)
{
    static u8 a = 0xA5, r;
    volatile u8 *m = (volatile u8 *)AUX_PROBE;
    *m = 0x5A;
    xfer(M_WRITE_AUX, (u16)&a, AUX_PROBE, 1);
    xfer(M_READ_AUX, AUX_PROBE, (u16)&r, 1);
    return (u8)(r == 0xA5 && *m == 0x5A);
}

/* Debranche /RAM de la liste des peripheriques ProDOS. 0 si elle contient des
 * fichiers (ou ne se laisse pas lire) : l'auxiliaire n'est alors pas a nous. */
static u8 take_ramdisk(void)
{
    u8 n = DEVCNT, i, k;
    unsigned char hdr[4 + 39];
    int fd;

    for (i = 0; i <= n; ++i)
        if (DEVLST[i] == RAMDISK)
            break;
    if (i > n)
        return 1;                           /* pas de /RAM */
    fd = open("/RAM", O_RDONLY);
    if (fd < 0)
        return 0;
    k = (u8)(read(fd, hdr, sizeof hdr) == (int)sizeof hdr && hdr[4 + 33] == 0 && hdr[4 + 34] == 0);
    close(fd);
    if (!k)
        return 0;
    for (; i < n; ++i)
        DEVLST[i] = DEVLST[i + 1];
    DEVCNT = (u8)(n - 1);
    return 1;
}

u8 aux_init(void)
{
    if ((MACHID & 0x30) != 0x30)            /* ProDOS ne signale pas 128 Ko */
        return 0;
    /* /RAM d'abord : son pilote vit dans la RAM auxiliaire, que l'installation
     * du noyau de copie ($0300) et le test ci-dessous ecraseraient. */
    if (!take_ramdisk())
        return 0;
    memcpy((void *)AUX_RT, aux_rt, (size_t)(aux_rt_end - aux_rt));
    xfer(M_WRITE_AUX, AUX_RT, AUX_RT, (u16)(aux_rt_end - aux_rt));
    return probe();
}

#endif
