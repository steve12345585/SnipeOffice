# SnipeOffice API IDL Files Except UDK API

Contains all of the IDL files except those in `udkapi`.

i.e. the interfaces that are specific to the SnipeOffice application.
An artificial (?) separation.

The reference `offapi/type_reference/offapi.idl` and
`udkapi/type_reference/udkapi.idl` (formerly combined into a single
`offapi/type_reference/types.rdb`) are used to detect inadvertent incompatible
changes.  They are plain-text `.idl` files (not strictly lexicographically sorted,
though, so they satisfy the `.idl` file requirements for no forward dependencies),
so in cases where we deliberately /do/ become incompatible they can be modified
manually.

Old such cases of deliberately becoming incompatible are listed in
`offapi/type_reference/typelibrary_history.txt`, newer such cases are recorded in
the `git log`s of (now superseded) `offapi/type_reference/types.rdb`,
`offapi/type_reference/offapi.rdb`, and `udkapi/type_reference/udkapi.rdb`, new such
cases are recorded in the `git log`s of `offapi/type_reference/offapi.idl` and
`udkapi/type_reference/udkapi.idl`.
