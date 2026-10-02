/* filesel.c -- selecteur de fichier / de dossier (voir filesel.h). */

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "filesel.h"
#include "window.h"

#ifdef __CC65__
#define MAKE_DIR(p) mkdir(p)
#else
#include <dirent.h>
#include <sys/stat.h>
#define MAKE_DIR(p) mkdir(p, 0777)
#endif

/* Code en carte langage (segment LC de cc65), comme dialog.c. */
#ifdef __CC65__
#pragma code-name (push, "LC")
#endif

#define CM_FS_PICK 250   /* Entree sur la liste */
#define CM_FS_NEW  251   /* bouton de creation de dossier */

#define PATH_MAX_LEN (FS_PATH_LEN - 1)

static char fs_cwd[PATH_MAX_LEN + 1];
static char fs_name[PATH_MAX_LEN + 1];   /* tampon de la saisie */
static char fs_dir[FS_DIR_MAX][17];
static u8   fs_count;
static u8   fs_mode;
static u8   fs_first;   /* premiere entree triee (0, ou 1 apres "../") */

#define FS_LBL    0
#define FS_IN     1
#define FS_CWD    2
#define FS_LIST   3
#define FS_OK     4
#define FS_CANCEL 5
#define FS_NEW    6

/* Les libelles ("" ici) sont poses apres dialog_build(), depuis l'appelant. */
static const TDlgItem fs_items[] = {
    { DI_LABEL,  2,  1,  6, "",      0 },
    { DI_INPUT,  8,  1, 27, fs_name, PATH_MAX_LEN - 1 },
    { DI_LABEL,  2,  3, 34, "",      0 },
    { DI_LIST,   2,  4, 34, 0,       8 },
    { DI_BUTTON, 2, 13, 10, "",      CM_OK },
    { DI_BUTTON, 26, 13, 10, "",     CM_CANCEL },
    { DI_BUTTON, 13, 13, 12, "",     CM_FS_NEW },
};

/* --- Repertoire --------------------------------------------------------- */

/* Ajoute name (sous-repertoire : avec '/') a sa place dans l'ordre
 * alphabetique (apres "../"), en ignorant les entrees cachees ; les fichiers
 * ne sont gardes que hors du mode dossier. */
static void dir_add(const char *name, u8 len, u8 is_dir)
{
    char tmp[17];
    u8 i;
    if (fs_count >= FS_DIR_MAX || len == 0 || len > 15 || name[0] == '.')
        return;
    if (!is_dir && fs_mode == FS_FOLDER)
        return;
    memcpy(tmp, name, len);
    if (is_dir)
        tmp[len++] = '/';
    tmp[len] = '\0';
    for (i = fs_first; i < fs_count && strcmp(fs_dir[i], tmp) < 0; ++i)
        ;
    memmove(fs_dir[i + 1], fs_dir[i], (u8)(fs_count - i) * 17U);
    memcpy(fs_dir[i], tmp, 17);
    ++fs_count;
}

/* Entrees triees (liste vide si le dossier est illisible), "../" en tete hors racine de volume. */
static void dir_read(void)
{
    fs_count = 0;
    if (strchr(fs_cwd + 1, '/'))      /* pas a la racine d'un volume */
        strcpy(fs_dir[fs_count++], "../");
    fs_first = fs_count;

#ifdef __CC65__
    /* Un repertoire ProDOS se lit comme un fichier : blocs de 512 octets (2
     * pointeurs de 2 octets, 13 entrees de 39 octets, 1 octet de bourrage),
     * lus par morceaux pour ne pas reserver un tampon de bloc. L'en-tete du
     * repertoire (type $E/$F) et les entrees effacees (type 0) sont ignores. */
    {
        unsigned char ent[39];
        int fd = open(fs_cwd, O_RDONLY);
        u8 e, st;
        if (fd < 0)
            return;
        while (read(fd, ent, 4) == 4) {           /* pointeurs de bloc */
            for (e = 0; e < 13 && read(fd, ent, 39) == 39; ++e) {
                st = (u8)(ent[0] >> 4);
                if (st != 0 && st != 0xE && st != 0xF)
                    dir_add((const char *)ent + 1, (u8)(ent[0] & 0x0F), st == 0xD);
            }
            read(fd, ent, 1);                     /* octet de bourrage du bloc */
        }
        close(fd);
    }
#else
    {
        DIR *d = opendir(fs_cwd[0] ? fs_cwd : ".");
        struct dirent *de;
        if (!d)
            return;
        while ((de = readdir(d)) != 0)
            dir_add(de->d_name, (u8)strlen(de->d_name), de->d_type == DT_DIR);
        closedir(d);
    }
#endif
}

/* Ajoute "/" + name (un '/' final est ignore) a dst, de capacite
 * PATH_MAX_LEN + 1 ; 0 = ok, dst inchange sinon. */
static u8 cat(char *dst, const char *name)
{
    size_t l = strlen(dst), n = strlen(name);
    if (name[n - 1] == '/')
        --n;
    if (l + n >= PATH_MAX_LEN)
        return 1;
    dst += l;
    *dst++ = '/';
    memcpy(dst, name, n);
    dst[n] = '\0';
    return 0;
}

/* Descend dans name ("../" ou "SOUS/") ; 0 = ok. */
static u8 dir_enter(const char *name)
{
    char *slash;
    if (name[0] != '.')
        return cat(fs_cwd, name);
    slash = strrchr(fs_cwd, '/');
    if (!slash || slash == fs_cwd)
        return 1;
    *slash = '\0';
    return 0;
}

/* Chemin complet de name dans full (FS_PATH_LEN octets) ; 0 = ok. */
static u8 join(char *full, const char *name)
{
    strcpy(full, fs_cwd);
    return cat(full, name);
}

static u8 path_exists(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return 0;
    close(fd);
    return 1;
}

/* --- Dialogue ----------------------------------------------------------- */

static const char *fs_get(void *ctx, u16 i)
{
    (void)ctx;
    return fs_dir[i];
}

static void fs_refresh(void)
{
    dir_read();
    list_set_count(&dlg_item[FS_LIST].list, fs_count);
    label_set(&dlg_item[FS_CWD].label, fs_cwd);
}

static void fs_pick(void)
{
    const char *n = fs_dir[dlg_item[FS_LIST].list.sel];
    if (fs_count == 0)
        return;
    if (n[strlen(n) - 1] == '/') {
        if (dir_enter(n) == 0)
            fs_refresh();
    } else {
        input_set_text(&dlg_item[FS_IN].input, n);
        dlg_win.result = CM_OK;
    }
}

/* Comme app_exec(), mais les commandes du dialogue sont traitees ici. */
static u8 fs_run(void)
{
    TEvent ev;
    app_modal_open(&dlg_win);
    while (dlg_win.result == 0) {
        app_wait_event(&ev);
        if (dlg_win.g.v.ops->handle(&dlg_win.g.v, &ev) == EVENT_NOT_HANDLED &&
            ev.type == EV_COMMAND) {
            if (ev.cmd == CM_FS_PICK)
                fs_pick();
            else if (ev.cmd == CM_FS_NEW)
                dlg_win.result = CM_FS_NEW;
        }
        app_redraw();
    }
    app_modal_close(&dlg_win);
    return dlg_win.result;
}

u8 filesel(u8 mode, const char *title, const TFileSelText *t, char *path)
{
    u8 r;
    char *slash;
    char full[PATH_MAX_LEN + 1];

    fs_mode = mode;
    fs_cwd[0] = '\0';
    if (mode == FS_FOLDER) {
        if (path[0] == '/')
            strcpy(fs_cwd, path);
        path[0] = '\0';
    } else if (path[0] == '/' && (slash = strrchr(path, '/')) != 0) {
        *slash = '\0';
        strcpy(fs_cwd, path);
        memmove(path, slash + 1, strlen(slash + 1) + 1);
    }
    if (fs_cwd[0] == '\0' && !getcwd(fs_cwd, sizeof fs_cwd))
        fs_cwd[0] = '\0';

    for (;;) {
        dialog_build(title, 38, 16, fs_items, (u8)(mode == FS_FOLDER ? 7 : 6), CM_OK);
        dlg_item[FS_LBL].label.text = t->name;
        dlg_item[FS_OK].button.text = t->ok;
        dlg_item[FS_CANCEL].button.text = t->cancel;
        if (mode == FS_FOLDER)
            dlg_item[FS_NEW].button.text = t->newdir;
        input_set_text(&dlg_item[FS_IN].input, path);
        dlg_item[FS_LIST].list.get = fs_get;
        dlg_item[FS_LIST].list.cmd = CM_FS_PICK;
        fs_refresh();
        r = fs_run();
#ifdef __CC65__
        for (slash = fs_name; *slash; ++slash)    /* ProDOS : noms en majuscules */
            if (*slash >= 'a' && *slash <= 'z')
                *slash -= 32;
#endif
        strcpy(path, fs_name);
        if (r == CM_CANCEL)
            return CM_CANCEL;

        if (r == CM_FS_NEW) {                /* creer fs_cwd/fs_name et y entrer */
            if (fs_name[0] && join(full, fs_name) == 0 && MAKE_DIR(full) == 0 &&
                dir_enter(fs_name) == 0)
                path[0] = '\0';
            else
                msgbox(title, t->error, MB_OK);
            continue;
        }

        if (mode == FS_FOLDER) {
            strcpy(path, fs_cwd);
            return CM_OK;
        }
        if (fs_name[0] == '\0')
            continue;
        if (join(full, fs_name) != 0)
            continue;
        if (mode == FS_SAVE && path_exists(full) &&
            msgbox(title, t->exists, MB_YESNO) != CM_YES)
            continue;
        strcpy(path, full);
        return CM_OK;
    }
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
