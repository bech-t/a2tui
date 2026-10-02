#!/bin/bash
# Mesures de taille pour docs/usage.*.md : table par module de la bibliotheque,
# puis empreinte memoire de la demo et de l'editeur (variantes comprises).
# Usage : tools/measure.sh   (depuis la racine du depot ; compile dans un
# dossier temporaire, ne touche pas a build/)
set -e
cd "$(dirname "$0")/.."
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT

make -s BUILDDIR="$T/std" lib dsk edit >/dev/null 2>&1 || make -s BUILDDIR="$T/std" lib edit >/dev/null
make -s BUILDDIR="$T/nomouse" TUI_MOUSE=0 edit >/dev/null
make -s BUILDDIR="$T/noaux" EDIT_AUX=0 edit >/dev/null

seg() { od65 --dump-segsize "$1" | awk -v s="$2" '$1==s":"{print $2}'; }

echo "| Module | CODE | RODATA | BSS | LC |"
echo "| --- | --- | --- | --- | --- |"
tc=0; tr=0; tb=0; tl=0
for o in "$T"/std/*.o; do
    n=$(basename "$o" .o)
    case "$n" in demo|edit_*) continue ;; esac
    c=$(seg "$o" CODE); r=$(seg "$o" RODATA); b=$(seg "$o" BSS); l=$(seg "$o" LC)
    c=${c:-0}; r=${r:-0}; b=${b:-0}; l=${l:-0}
    printf "| %s | %s | %s | %s | %s |\n" "$n" "$c" "$r" "$b" "$l"
    tc=$((tc+c)); tr=$((tr+r)); tb=$((tb+b)); tl=$((tl+l))
done
printf "| **Total** | **%s** | **%s** | **%s** | **%s** |\n" "$tc" "$tr" "$tb" "$tl"
echo
echo "(modules lies seulement s'ils sont references ; ld65 elague par fichier objet)"
echo

# Fin du BSS (adresse) et taille du segment LC depuis une carte du lieur.
bss_end() { awk '/^BSS /{print $3}' "$1"; }
lc_size() { awk '/^LC /{print $4}' "$1"; }
free_main() { printf "%d" $((0xBB00 - 0x$(bss_end "$1") - 1)); }

demo=$(ls "$T"/std/tuidemo.map 2>/dev/null || true)
if [ -n "$demo" ]; then
    echo "Demo : programme \$0C00-\$$(bss_end "$demo"), LC \$$(lc_size "$demo") octets"
fi
echo "Editeur (RAM principale seule, tampon de texte) :"
echo "  avec souris          : $(free_main "$T/std/edit.map") octets (programme \$0C00-\$$(bss_end "$T/std/edit.map"))"
echo "  sans souris          : $(free_main "$T/nomouse/edit.map") octets"
echo "  EDIT_AUX=0 (souris)  : $(free_main "$T/noaux/edit.map") octets"
echo "  LC                   : \$$(lc_size "$T/std/edit.map") octets sur \$C00"
echo "Editeur avec RAM auxiliaire : 47104 octets (AUX_CAP)"
