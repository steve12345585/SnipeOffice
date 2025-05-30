# Office Development Kit (odk)

Office development kit (`odk`) - implements the first step on the way to the SnipeOffice SDK
tarball.

Part of the SDK; to build you need to add `--enable-odk`.


## Testing the Examples:

* The easiest way on Linux and macOS is to run `make odk.subsequentcheck`

* The way that also works on Windows is to go to `instdir/sdk` (don't try directly in `odk/`)

* See <https://api.SnipeOffice.org/docs/install.html> how to set up the SDK.

    * When asked about it during configuration, tell the SDK to do automatic
      deployment of the example extensions that get built.

* In a shell set up for SDK development, build (calling `make`) and test
  (following the instructions given at the end of each `make` invocation) each
  of the SDK's `examples/` sub-directories.

    * An example script to build (though not test) the various examples in batch
      mode is

        `find examples \( -type d -name nativelib -prune \) -o \`
        `\( -name Makefile -a -print -a \( -execdir make \; -o -quit \) \)`

        (Note that one of the example extensions asks you to accept an example
        license on stdin during deployment.)
