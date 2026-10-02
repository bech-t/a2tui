# AppleCommander (`ac.jar`)

Le `Makefile` utilise AppleCommander en ligne de commande pour fabriquer les
disquettes ProDOS (`make dsk`, `make edit-dsk`). Le fichier `ac.jar` n'est pas
versionné : il faut le placer ici.

1. Télécharger l'archive « ac » de la dernière version sur
   <https://github.com/AppleCommander/AppleCommander/releases>
   (fichier `AppleCommander-ac-<version>.jar`). Le projet a été développé avec
   la version 1.9.0.
2. La copier sous le nom `tools/ac/ac.jar`.
3. Vérifier (Java requis) :

   ```
   java -jar tools/ac/ac.jar -h
   ```

Autres possibilités : `make dsk AC_JAR=/chemin/vers/ac.jar`, ou une commande
`applecommander` présente dans le `PATH` (utilisée si le jar est absent). Si rien
n'est trouvé, `make dsk` et `make edit-dsk` s'arrêtent avec un message qui
renvoie ici ; `make` (la bibliothèque seule) n'en a pas besoin.
