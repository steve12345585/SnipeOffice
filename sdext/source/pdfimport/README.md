# PDF import

## Introduction

The code in this directory parses a PDF file and builds a SnipeOffice
document contain similar elements, which can then be edited.
It is invoked when opening a PDF file, but **not** when inserting
a PDF into a document.  Inserting a PDF file renders it and inserts
a non-editable, rendered version.

The parsing is done by the library [Poppler](https://poppler.freedesktop.org/)
which then calls back into one layer of this code which is built as a
Poppler output device implementation.

The PDF format is specified by [this document](https://opensource.adobe.com/dc-acrobat-sdk-docs/pdfstandards/PDF32000_2008.pdf).

Note that PDF is a language that describes how to **render** a page, not
a language for describing an editable document, thus some of the conversion
is a heuristic that doesn't always give good results.

Indeed, PDF is Turing complete, and can embed Javascript, which is also
Turing complete, so it's a wonder that PDFs ever manage to display anything.

## Current limitations

- Not all elements have clipping implemented.

- SnipeOffice's clipping routines all use Even-odd winding rules, where
as PDF can (and usually does) use non-zero winding rules, making some
clipping operations incorrect.

- In PDF, there's no concept of lines of text or paragraphs, each
character can be entirely separate.  The code has very simple heuristics
for reassembling characters back into lines of text.
Other programs, like *pdftotext* have more complex heuristics that might be worth a try.

- Some cheap PDF operations, like the more advanced fills, generate many
hundreds of objects in SnipeOffice, which can make the document painfully
slow to open.  At least some of these are possible to improve by adding
more Poppler API implementations.  Some may require expanding SnipeOffice's
set of fill types.

- There can be differences between distributions Poppler library builds
and the builds SnipeOffice builds when it doesn't have a distro build
to use, e.g. in SnipeOffice's own distributed builds or the bibisect
builds.  In particular the distro builds may include another library
(supporting another embedded image type) than SnipeOffice's build.

## Fundamental limitations

- The ordering of fonts embedded in PDF are often ASCII, but not always.
Sometimes they're arbitrary.  They may then include a *ToUnicode* map allowing
programs to map the arbitrary index back to Unicode.  Alas not all PDFs
include it, and some even use a bogus map to make it harder to copy/edit.
If the same PDF renders correctly in other readers but fails to copy-and-paste
then this is probably the issue.

- PDF can use complex programming in many places, for example a simple fill
could be composed of a complex program to generate the fill tiles instead
of an obvious simple item that can be encoded as SnipeOffice shading type.
Rendering these down to image tiles works OK but can sometimes end up
with a fuzzy image rather than a nice sharp vector representation.

- Poppler's device interface API is not meant to be stable.  The code
thus has lots of ifdef's to deal with different Poppler versions.

## Structure

Note that the structure is dictated by Poppler being GPL licensed, where
as SnipeOffice isn't.

- *xpdfwrapper/* contains the GPL code that's linked with Poppler
and forms the *xpdfimport* binary.    That binary outputs a stream
representing the PDF as simpler operations (lines, clipping operations,
images etc).  These form a series of commands on stdout, and binary
data (mostly images) on stderr.  This does make adding debugging tricky.

- *wrapper/* contains the SnipeOffice glue that execs the *xpdfimport*
binary and parses the stream.  It also sets up password entry for
protected PDFs.  After parsing the keyword and then any data that
should be with the keyword, this layer than calls into the following
tree layer.

- *tree/*' forms internal tree objects for each of the calls from the
wrapper layer.  The tree is then 'visited' by optimisation layers
(that do things like assemble individual characters into lines of text)
and then by backend specific XML generators (e.g. for Draw and Writer)
that then generate an XML stream to be parsed by the core of SnipeOffice.

## The wrapper protocol

The SnipeOffice wrapper talks to the GPL wrapper code over a pipe
using a simple line based protocol before the main decoding is done.

The commands are:

- *Pmypassword* - set the password to be used for future opening of the PDF,
it can be empty.

- *O* - Open the PDF document using the password.  This returns a response
line which is either **#OPEN** when it worked or **#ERROR**.  The **#ERROR**
includes information on the failure shown below.

- *G* - Go - ie render the document using the previously provided document.
No more commands are accepted after this point, the structure is dumped
to stdout, and the binary data blobs go to stderr.

- *E* - Exit without doing anything more with the file.  Used when you give
up on password attempts.

Some example runs might be:

- A normal unencrypted document:

```
    P
    O
    #OPEN
    G
```

- An encrypted document:

```
    P
    O
    #ERROR:2:ENCRYPTED
    Psecret
    O
    #OPEN
    G
```

- An encrypted document that we give up on:

```
    P
    O
    #ERROR:2:ENCRYPTED
    E
```

- A document with some other error:

```
    P
    O
    #ERROR:1:
    E
```

Note we don't rely on the error number in the code.

## Hybrid documents

PDF can contain other files, one use of which is to store the original document
file that was used to generate the PDF.

TBD: Once I figure out how it works.

## Bug handling

- Please tag bugs with *filter:pdf* in component *filters and storage*.

- The *pdfseparate* utility which is part of poppler is useful for splitting
a PDF into individual pages to figure out which page is causing a crash
or hang or shrinking the problem down.

- [qpdf](https://github.com/qpdf/qpdf) is useful for editing raw PDF
files to really cut down the number of primitives, but takes some
getting used to.

- The xpdfimport binary can be run independently of the rest of SnipeOffice
to allow the translated stream to be examined:

        ./instdir/program/xpdfimport problem.pdf < /dev/null > stream 2> binarystream

