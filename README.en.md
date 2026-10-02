# a2tui

[Français](README.md)

A C ([cc65](https://cc65.github.io/)) text-mode user interface library in the
style of Turbo Vision for the **Apple //e and //c**: drop-down menus, windows,
dialogs, buttons, input lines, lists, check boxes, a progress bar, a file
selector and mouse support. 40 or 80 columns, MouseText when the machine
allows it, all in static memory (no `malloc`).

![a2tui demo in 80 columns (Apple //e)](docs/demo.png)

The repository also ships a demo and a **text editor** (`apps/edit`, in the
spirit of MS-DOS EDIT) that uses auxiliary RAM when it is present.

## Try it

```
make            # build/a2tui.lib
make dsk        # build/tuidemo.dsk: bootable ProDOS disk (the demo)
make edit-dsk   # build/edit.dsk: the text editor
```

Requirements: cc65, Java and AppleCommander ([`tools/ac/README.md`](tools/ac/README.md)).
The disks run on an Apple //e or //c, real or emulated.

## Documentation

Usage guide and technical notes: [English](docs/usage.en.md) ·
[français](docs/usage.fr.md).

## License

MIT, see [`LICENSE`](LICENSE). The files in `prodos/` are those of ProDOS
2.4.2 (open source); AppleCommander is not included.
