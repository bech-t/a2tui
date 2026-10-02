/* textbuf.c -- gap buffer, en RAM principale ou, si l'Apple II en a une, en
 * RAM auxiliaire (auxmem.h).
 *
 * Mode auxiliaire : les octets ne sont pas adressables directement. Une
 * fenetre de 256 octets en RAM principale (cache) donne l'acces en lecture ;
 * elle est invalidee par toute ecriture. */

#include <string.h>
#include "textbuf.h"
#ifndef EDIT_AUX
#define EDIT_AUX 1
#endif
#if EDIT_AUX
#include "auxmem.h"
#endif

#define CACHE_SZ 256

u16 tb_cap;
static u8 *B;                  /* debut du tampon (mode principal) */
static u16 gs;                 /* debut du trou */
static u16 gl;                 /* longueur du trou */
#if EDIT_AUX
static u8 use_aux;
static u8 cache[CACHE_SZ];     /* copie de [cbase, cbase + clen) du tampon */
static u16 cbase, clen;
#else
#define use_aux 0
#endif

#ifdef TUI_HOST
#define HOST_CAP 14336u
static u8 host_mem[HOST_CAP];
#else
extern const u16 tb_mem_start, tb_mem_end;     /* memmap.s */
#endif

void tb_init(void)
{
#if EDIT_AUX
    use_aux = aux_init();
    if (use_aux) {
        tb_cap = AUX_CAP;
        tb_clear();
        return;
    }
#endif
#ifdef TUI_HOST
    B = host_mem;
    tb_cap = HOST_CAP;
#else
    B = (u8 *)tb_mem_start;
    tb_cap = (u16)(tb_mem_end - tb_mem_start);
#endif
    tb_clear();
}

void tb_clear(void)
{
    gs = 0;
    gl = tb_cap;
#if EDIT_AUX
    clen = 0;
#endif
}

u16 tb_len(void)  { return (u16)(tb_cap - gl); }
u16 tb_free(void) { return gl; }

#if EDIT_AUX
/* Place [p, p + n) du tampon dans le cache. */
static void load_win(u16 p, u16 n)
{
    aux_read(cache, p, n);
    cbase = p;
    clen = n;
}
#endif

u8 tb_at(u16 i)
{
    u16 p = i < gs ? i : (u16)(i + gl);
#if EDIT_AUX
    if (use_aux) {
        if ((u16)(p - cbase) >= clen) {
            u16 n = (u16)(tb_cap - p);
            load_win(p, n > CACHE_SZ ? CACHE_SZ : n);
        }
        return cache[p - cbase];
    }
#endif
    return B[p];
}

static void move_gap(u16 pos)
{
#if EDIT_AUX
    if (use_aux) {
        clen = 0;
        if (pos < gs)
            aux_move((u16)(pos + gl), pos, (u16)(gs - pos));
        else if (pos > gs)
            aux_move(gs, (u16)(gs + gl), (u16)(pos - gs));
        gs = pos;
        return;
    }
#endif
    if (pos < gs)
        memmove(B + pos + gl, B + pos, (size_t)(gs - pos));
    else if (pos > gs)
        memmove(B + gs, B + gs + gl, (size_t)(pos - gs));
    gs = pos;
}

u8 tb_insert(u16 pos, const u8 *s, u16 n)
{
    if (n > gl)
        return 0;
    move_gap(pos);
#if EDIT_AUX
    if (use_aux) {
        aux_write(gs, s, n);
        clen = 0;
    } else
#endif
        memcpy(B + gs, s, n);
    gs += n;
    gl -= n;
    return 1;
}

void tb_delete(u16 pos, u16 n)
{
    move_gap(pos);
    gl += n;
}

void tb_copy(u16 pos, u16 n, u8 *dst)
{
    while (n--)
        *dst++ = tb_at(pos++);
}

const u8 *tb_ptr(u16 pos, u16 *n)
{
    u16 p, e;
    if (pos < gs) {
        p = pos;
        e = gs;
    } else {
        p = (u16)(pos + gl);
        e = tb_cap;
    }
#if EDIT_AUX
    if (use_aux) {
        if ((u16)(p - cbase) >= clen)
            load_win(p, (u16)(e - p) > CACHE_SZ ? CACHE_SZ : (u16)(e - p));
        *n = (u16)(cbase + clen - p);
        if (*n > (u16)(e - p))
            *n = (u16)(e - p);
        return cache + (p - cbase);
    }
#endif
    *n = (u16)(e - p);
    return B + p;
}

/* Octet i et les octets qui le precedent dans son segment : pointeur sur
 * l'octet i, *k = nombre d'octets lisibles en reculant (i compris). */
static const u8 *tb_back(u16 i, u16 *k)
{
    u16 p, s;
    if (i < gs) {
        p = i;
        s = 0;
    } else {
        p = (u16)(i + gl);
        s = (u16)(gs + gl);
    }
    *k = (u16)(p - s + 1);
#if EDIT_AUX
    if (use_aux) {
        if (*k > CACHE_SZ)
            *k = CACHE_SZ;
        if ((u16)(p - cbase) >= clen)
            load_win((u16)(p + 1 - *k), *k);
        else if ((u16)(p - cbase) + 1 < *k)
            *k = (u16)(p - cbase + 1);
        return cache + (p - cbase);
    }
#endif
    return B + p;
}

u16 tb_line_start(u16 pos)
{
    u16 i, k;
    const u8 *p;
    for (;;) {
        if (pos == 0)
            return 0;
        i = (u16)(pos - 1);
        p = tb_back(i, &k);
        while (k--) {                   /* remonte le segment octet par octet */
            if (*p == '\r')
                return (u16)(i + 1);
            --p;
            --i;
        }
        pos = (u16)(i + 1);             /* i a fait le tour : debut du segment */
    }
}

u16 tb_line_end(u16 pos)
{
    u16 len = tb_len(), n;
    const u8 *p, *q;
    while (pos < len) {
        p = tb_ptr(pos, &n);
        q = (const u8 *)memchr(p, '\r', n);
        if (q)
            return (u16)(pos + (q - p));
        pos = (u16)(pos + n);
    }
    return len;
}

u16 tb_count_cr(u16 from, u16 to)
{
    u16 cnt = 0, n, k;
    const u8 *p, *q;
    while (from < to) {
        p = tb_ptr(from, &n);
        if (n > (u16)(to - from))
            n = (u16)(to - from);
        from = (u16)(from + n);
        k = n;
        while (k && (q = (const u8 *)memchr(p, '\r', k)) != 0) {
            ++cnt;
            k = (u16)(k - (u16)(q - p) - 1);
            p = q + 1;
        }
    }
    return cnt;
}

static u8 lower(u8 c)
{
    return (c >= 'A' && c <= 'Z') ? (u8)(c + 32) : c;
}

u16 tb_find(u16 from, const u8 *pat, u8 n, u8 match_case)
{
    u16 len = tb_len(), i;
    u8 k;
    if (n == 0 || len < n)
        return TB_NONE;
    for (; from + n <= len; ++from) {
        for (k = 0; k < n; ++k) {
            i = (u16)(from + k);
            if (match_case ? tb_at(i) != pat[k] : lower(tb_at(i)) != lower(pat[k]))
                break;
        }
        if (k == n)
            return from;
    }
    return TB_NONE;
}

/* Chargement : lire dans le tampon rendu par tb_load_buf() (room octets), puis
 * tb_load_commit(n) avec n <= room octets utiles ; room = 0 : tampon plein. */
u8 *tb_load_buf(u16 *room)
{
    u16 free_ = (u16)(tb_cap - gs);
#if EDIT_AUX
    if (use_aux) {
        *room = free_ > CACHE_SZ ? CACHE_SZ : free_;
        clen = 0;
        return cache;
    }
#endif
    *room = free_;
    return B + gs;
}

void tb_load_commit(u16 n)
{
#if EDIT_AUX
    if (use_aux)
        aux_write(gs, cache, n);
#endif
    gs += n;
    gl -= n;
}
