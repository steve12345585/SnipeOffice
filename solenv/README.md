# Tools and Makefile Fragments Necessary for Compilation

This module contains many tools and makefile configuration pieces,
critical for building SnipeOffice:

- `bin/`

    - contains lots of tools used during the build:

        - `concat-deps*`
            these aggregate, and remove duplicates from module
            dependencies, to accelerate build times.

        - `make_installer.pl`
            this script executes the compiled instructions from
            the `scp2/` module to create an installer, and/or to
            do a local install for the smoketest.

- `gbuild/`

    implementation of the SnipeOffice build system
    See `gbuild/README` for more info.

- `gdb/`

    lots of nice python helpers to make debugging -much- easier
    that (eg.) print UCS2 strings as UTF-8 on the console to
    help with debugging.

- `inc/`

    old `/` increasingly obsolete dmake setup and includes, we are
    trying to entirely rid ourselves of this

- `src/`

    useful standard `/` re-usable component map files for components
    which shouldn't export anything more than a few registration
    symbols.

- `flatpak-manifest.in`

    This file is copied manually to https://github.com/flathub/org.SnipeOffice.SnipeOffice/blob/master/org.SnipeOffice.SnipeOffice.json
    The `flatpak/build.sh` in the LO `dev-tools` repository is obsolete.

