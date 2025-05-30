# SnipeOffice Filters

Filter registration and some simple filters (also descriptions).

Desperate splitting of code into small shared libraries for historical
reasons presumably (OS/2 and Windows 3.x). The libraries produced from
the code in each subdirectory of `filter/source/graphicfilter` are
graphic format import or export filters. But they don't have uniform
API. Some have either a `GraphicImport` or `GraphicExport` entry point,
and are loaded and used in a uniform fashion from code in
`svtools/source/filter/filter.cxx`. Others have different API and are
loaded from other places. For instance `icgm` has `ImportCGM`, and is
loaded and used by `sd/source/filter/cgm/sdcgmfilter.cxx` (!).
Svgreader is used for "File -> Open" and then to choose the svg file.
For "Insert -> Picture -> From File", see `svgio/source/svgreader` directory.

## Filter Configuration

The filter configuration consists of two parts, the type definition in
`filter/source/config/fragments/types/` and the actual filter definition
in `filter/source/config/fragments/filters/`.

Each file type e.g. text file should be represented by exactly one
type definition. This type can then be referenced by several different
filters, e.g. calc text, writer text.
