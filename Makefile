# a2tui -- bibliotheque TUI (cc65) pour Apple II.
#
#   make            -> build/a2tui.lib          (bibliotheque)
#   make dsk        -> build/tuidemo.dsk        (disquette ProDOS bootable)
#   make size       -> taille par module

TARGET   := apple2
CL65     := cl65
AR65     := ar65

AC_JAR     ?= tools/ac/ac.jar
# AppleCommander : le jar (AC_JAR) s'il existe, sinon la commande `applecommander`
# du PATH (voir tools/ac/README.md).
ifneq ($(wildcard $(AC_JAR)),)
AC := java -jar $(AC_JAR)
else ifneq ($(shell command -v applecommander 2>/dev/null),)
AC := applecommander
endif
PRODOS_TPL := prodos/prodos.dsk
LOADER     := prodos/loader.system
# Extrait de ProDOS 2.4.2. Requis par Bitsy Bye pour lancer un BIN ; ajoute EN
# DERNIER sur le disque : ProDOS demarre le premier *.SYSTEM du
# repertoire, qui doit rester celui de l'application.
BASICSYS   := prodos/basic.system

# Le loader ProDOS charge le binaire a son adresse de lien. Le tampon de fichier
# ProDOS (1 Ko, un seul fichier ouvert a la fois) est fixe en $0800-$0BFF par
# apple2-iobuf-0800.o : le programme commence donc en $0C00, et il n'y a ni tas
# ni malloc. HIMEM $BF00 = la page globale ProDOS.
LOADADDR := 0x0C00
HIMEM    ?= 0xBF00
CC65_LIB ?= /usr/share/cc65/lib
IOBUF    := $(CC65_LIB)/apple2-iobuf-0800.o

SRCDIR   := src
INCDIR   := include
BUILDDIR := build

LIBSRC := $(wildcard $(SRCDIR)/*.c)
LIBASM := $(wildcard $(SRCDIR)/*.s)
LIBOBJ := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(LIBSRC)) \
          $(patsubst $(SRCDIR)/%.s,$(BUILDDIR)/%.o,$(LIBASM))
LIB    := $(BUILDDIR)/a2tui.lib

# Pas de -Cl : les groupes imbriques (bureau -> fenetre) rendent group_draw_children
# et handle() re-entrants ; des locales statiques y seraient corrompues.
CFLAGS  := -t $(TARGET) -O -Os -I $(INCDIR)
# make TUI_MOUSE=0 : binaire sans souris (environ 2,8 Ko de moins)
ifdef TUI_MOUSE
CFLAGS  += -DTUI_MOUSE=$(TUI_MOUSE)
endif
LDFLAGS := -t $(TARGET) --start-addr $(LOADADDR) -Wl -D,__HIMEM__=$(HIMEM),-D,__STACKSIZE__=0x0400

PROGRAM := TUIDEMO
BIN     := $(BUILDDIR)/tuidemo.bin
DSK     := $(BUILDDIR)/tuidemo.dsk

.PHONY: all lib dsk size clean edit edit-dsk check-ac
all: lib
lib: $(LIB)

$(LIB): $(LIBOBJ)
	rm -f $@
	$(AR65) a $@ $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(wildcard $(INCDIR)/*.h) | $(BUILDDIR)
	$(CL65) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/%.o: $(SRCDIR)/%.s | $(BUILDDIR)
	$(CL65) -t $(TARGET) -c -o $@ $<

$(BUILDDIR)/demo.o: demo/demo.c $(wildcard $(INCDIR)/*.h) | $(BUILDDIR)
	$(CL65) $(CFLAGS) -c -o $@ $<

$(BIN): $(BUILDDIR)/demo.o $(LIB)
	$(CL65) $(LDFLAGS) -m $(BUILDDIR)/tuidemo.map -o $@ $^

# Les disquettes ont besoin d'AppleCommander.
check-ac:
ifeq ($(AC),)
	@echo "AppleCommander introuvable : ni $(AC_JAR), ni la commande 'applecommander' dans le PATH." >&2
	@echo "Voir tools/ac/README.md (ou : make AC_JAR=/chemin/vers/ac.jar)." >&2
	@false
endif

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

dsk: $(DSK)
$(DSK): $(BIN) $(LOADER) $(PRODOS_TPL) $(BASICSYS) | check-ac
	cp $(PRODOS_TPL) $@
	$(AC) -n  $@ TUIDEMO
	$(AC) -as $@ $(PROGRAM)          < $(BIN)
	$(AC) -p  $@ $(PROGRAM).SYSTEM sys < $(LOADER)
	@echo "Disquette prete : $@"
	$(AC) -l $@
	$(AC) -p  $@ BASIC.SYSTEM sys < $(BASICSYS)

# --- Editeur de texte (apps/edit) -------------------------------------------
# make EDIT_AUX=0 : sans RAM auxiliaire (tampon de texte toujours en RAM principale)
EDIT_AUX ?= 1
EDIT_SRC := $(wildcard apps/edit/*.c)
EDIT_ASM := $(wildcard apps/edit/*.s)
ifeq ($(EDIT_AUX),0)
EDIT_SRC := $(filter-out apps/edit/auxmem.c,$(EDIT_SRC))
EDIT_ASM := $(filter-out apps/edit/auxrt.s,$(EDIT_ASM))
endif
EDIT_OBJ := $(patsubst apps/edit/%.c,$(BUILDDIR)/edit_%.o,$(EDIT_SRC)) \
            $(patsubst apps/edit/%.s,$(BUILDDIR)/edit_%.o,$(EDIT_ASM))
EDIT_BIN := $(BUILDDIR)/edit.bin
EDIT_DSK := $(BUILDDIR)/edit.dsk

edit: $(EDIT_BIN)
edit-dsk: $(EDIT_DSK)

$(BUILDDIR)/edit_%.o: apps/edit/%.c $(wildcard apps/edit/*.h) $(wildcard $(INCDIR)/*.h) | $(BUILDDIR)
	$(CL65) $(CFLAGS) -DEDIT_AUX=$(EDIT_AUX) -I apps/edit -c -o $@ $<

$(BUILDDIR)/edit_%.o: apps/edit/%.s | $(BUILDDIR)
	$(CL65) -t $(TARGET) -c -o $@ $<

$(EDIT_BIN): $(EDIT_OBJ) $(LIB)
	$(CL65) $(LDFLAGS) -m $(BUILDDIR)/edit.map -o $@ $^ $(IOBUF)

$(EDIT_DSK): $(EDIT_BIN) $(LOADER) $(PRODOS_TPL) $(BASICSYS) apps/edit/README.TXT | check-ac
	cp $(PRODOS_TPL) $@
	$(AC) -n  $@ EDIT
	$(AC) -as $@ EDIT < $(EDIT_BIN)
	$(AC) -p  $@ EDIT.SYSTEM sys < $(LOADER)
	$(AC) -ptx $@ README.TXT < apps/edit/README.TXT
	@echo "Disquette prete : $@"
	$(AC) -l $@
	$(AC) -p  $@ BASIC.SYSTEM sys < $(BASICSYS)

size: $(LIBOBJ)
	@for o in $(LIBOBJ); do printf "%-22s" $$(basename $$o); \
	  size $$o 2>/dev/null | tail -1 >/dev/null; \
	  od65 --dump-segsize $$o | awk '/CODE|RODATA|DATA|BSS/{printf "%s=%s ", $$1, $$NF}'; echo; done

clean:
	rm -rf $(BUILDDIR)

# Regles locales facultatives (non versionnees).
-include local.mk
