/* edfile.c -- chargement, sauvegarde, liste de repertoire. */

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "edfile.h"
#include "textbuf.h"

#ifdef __CC65__
#include <errno.h>
#include <apple2.h>
#define OS_ERR() (ed_os_error = _oserror)
#else
#define OS_ERR() (ed_os_error = 0)
#endif

u8 ed_os_error;

u8 ed_load_file(const char *name)
{
    int fd = open(name, O_RDONLY), n;
    u8 *p, extra, c, last_cr = 0;
    u16 room, i, j;

    if (fd < 0) {
        OS_ERR();
        return ED_ERR_OPEN;
    }
    tb_clear();
    for (;;) {
        p = tb_load_buf(&room);
        if (room == 0) {                         /* plein : reste-t-il des octets ? */
            n = read(fd, &extra, 1);
            if (n > 0) {
                close(fd);
                tb_clear();
                return ED_ERR_BIG;
            }
            break;
        }
        n = read(fd, p, room);
        if (n < 0) {
            close(fd);
            tb_clear();
            return ED_ERR_IO;
        }
        if (n == 0)
            break;
        /* Normalisation sur place : bit 7 efface, CR/LF/CRLF -> CR, NUL -> espace. */
        for (i = j = 0; i < (u16)n; ++i) {
            c = (u8)(p[i] & 0x7F);
            if (c == '\n') {
                if (last_cr) {
                    last_cr = 0;
                    continue;
                }
                c = '\r';
            } else {
                last_cr = (c == '\r');
                if (c == 0)
                    c = ' ';
            }
            p[j++] = c;
        }
        tb_load_commit(j);
    }
    close(fd);
    return ED_OK;
}

u8 ed_save_file(const char *name)
{
    const u8 *p;
    u16 pos = 0, len = tb_len(), n;
    int fd;
#ifdef __CC65__
    _filetype = PRODOS_T_TXT;                /* fichier texte ProDOS ($04), pas BIN */
    _auxtype = 0;
#endif
    fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        OS_ERR();
        return ED_ERR_OPEN;
    }
    while (pos < len) {
        p = tb_ptr(pos, &n);
        if (write(fd, p, n) != (int)n) {
            close(fd);
            return ED_ERR_IO;
        }
        pos = (u16)(pos + n);
    }
    return close(fd) < 0 ? ED_ERR_IO : ED_OK;
}

