/* lang.c -- voir lang.h. */
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "lang.h"

/* Doit tenir "CLE=" + la valeur encore ECHAPPEE (chaque "\n" y prend 2
 * caracteres, un de plus que le vrai saut de ligne qu'il produira). La plus
 * longue ligne attendue fait 81 caracteres -- une traduction plus longue que
 * l'anglais source peut depasser une limite dimensionnee seulement sur ce
 * dernier, donc il faut verifier avec les vraies traductions, pas seulement
 * des textes courts. */
#define LANG_LINE_MAX 100

/* Lit une ligne dans buf, tronquee a max-1 ; renvoie sa longueur, ou 0xFF en
 * fin de fichier sans rien lu. Fin de ligne = CR ou LF (l'un ou l'autre,
 * jamais les deux consommes ensemble) : ProDOS (AppleCommander -ptx) n'ecrit
 * que du CR seul sur le fichier reellement pose sur le disque, les fichiers
 * edites sur PC du LF seul ou du CRLF -- dans ce dernier cas le second octet
 * du couple redevient une ligne vide au tour suivant, deja ignoree par
 * lang_load(). Un lecteur LF-seul n'accepterait jamais un fichier CR-seul :
 * il avalerait tout le fichier comme une seule "ligne". */
static u8 read_line(int fd, char *buf, u8 max)
{
    u8 n = 0;
    char c;

    for (;;) {
        if (read(fd, &c, 1) != 1)
            return n == 0 ? 0xFF : n;
        if (c == '\n' || c == '\r')
            break;
        if (n < (u8)(max - 1))
            buf[n++] = c;
    }
    buf[n] = '\0';
    return n;
}

static char *trim(char *s)
{
    char *end;
    while (*s == ' ' || *s == '\t')
        s++;
    end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t'))
        *--end = '\0';
    return s;
}

/* Copie src dans dst (borne a cap-1 + NUL), en depliant "\n" (deux
 * caracteres, backslash+n) en un vrai saut de ligne -- seule facon
 * d'exprimer un message multi-lignes (ex. MSG_NO_CARD, i18n.c) dans un
 * fichier "cle=valeur" a une entree par ligne. */
static void copy_value(char *dst, const char *src, u8 cap)
{
    u8 n = 0;
    while (*src && n < (u8)(cap - 1)) {
        if (src[0] == '\\' && src[1] == 'n') {
            dst[n++] = '\n';
            src += 2;
        } else {
            dst[n++] = *src++;
        }
    }
    dst[n] = '\0';
}

u8 lang_load(const char *path, LangEntry *table, u8 count)
{
    static char line[LANG_LINE_MAX];
    int fd = open(path, O_RDONLY);
    u8 len, i;
    char *eq, *key, *val;

    if (fd < 0)
        return 1;
    for (;;) {
        len = read_line(fd, line, LANG_LINE_MAX);
        if (len == 0xFF)
            break;
        if (len == 0 || line[0] == '#' || line[0] == ';')
            continue;
        eq = strchr(line, '=');
        if (!eq)
            continue;
        *eq = '\0';
        key = trim(line);
        val = trim(eq + 1);
        for (i = 0; i < count; ++i) {
            if (strcmp(table[i].key, key) == 0) {
                copy_value(table[i].buf, val, table[i].buf_len);
                break;
            }
        }
    }
    close(fd);
    return 0;
}
