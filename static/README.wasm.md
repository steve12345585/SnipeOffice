# Support for Emscripten Cross Build

This subdirectory provides support for building SnipeOffice as WASM, with the Emscripten toolchain.

You can build SnipeOffice for WASM for two separate purposes: 1)
Either to produce a WASM binary of SnipeOffice as such, using Qt5 for
its GUI, or 2) just compiling SnipeOffice core ("SnipeOffice
Technology") to WASM without any UI for use in other software that
provides the UI, like Collabora Online built as WASM.

The first purpose was the original reason for the WASM port and this
document was originally written with that in mind. For the second
purpose, look towards the end of the document for the section
"Building headless SnipeOffice as WASM for use in another product".

## Status of SnipeOffice as WASM with Qt

Configure `--with-package-format=emscripten` to have `workdir/installation/SnipeOffice/emscripten`
populated with just the relevant files from `instdir`.

The build generates a Writer-only LO build. You should be able to run either

    $ emrun --hostname 127.0.0.1 --serve_after_close workdir/installation/SnipeOffice/emscripten/qt_soffice.html
    $ emrun --hostname 127.0.0.1 --serve_after_close workdir/LinkTarget/Executable/qt_vcldemo.html

REMINDER: Always start new tabs in the browser, reload might fail / cache!

## Setup for the LO WASM build (with Qt)

We're using Qt 5.15.2 with Emscripten 3.1.46. There are a bunch of Qt patches
to fix the most grave bugs. Also there's rapid development in Emscripten, so
using another version often causes arbitrary problems.

- See below under Docker build for another build option

### Setup emscripten

<https://emscripten.org/docs/getting_started/index.html>

    git clone https://github.com/emscripten-core/emsdk.git
    ./emsdk install 3.1.46
    ./emsdk activate 3.1.46

Example `bashrc` scriptlet:

    EMSDK_ENV=$HOME/Development/SnipeOffice/git_emsdk/emsdk_env.sh
    [ -f "$EMSDK_ENV" ] && \. "$EMSDK_ENV" 1>/dev/null 2>&1

### Setup Qt

<https://doc.qt.io/qt-5/wasm.html>

Most of the information from <https://doc.qt.io/qt-6/wasm.html> is still valid for Qt5;
generally the Qt6 WASM documentation is much better, because it incorporated many
information from the Qt Wiki.

FWIW: Qt 5.15 LTS is not maintained publicly and Qt WASM has quite a few bugs. Most
WASM fixes from Qt 6 are needed for Qt 5.15 too. Allotropia offers a Qt repository
with the necessary patches cherry-picked.

With "-opensource -confirm-license" you agree to the open source license.

    git clone https://github.com/allotropia/qt5.git
    cd qt5
    git checkout 5.15.2+wasm
    ./init-repository --module-subset=qtbase
    ./configure -opensource -confirm-license -xplatform wasm-emscripten -feature-thread -prefix <whatever> QMAKE_CFLAGS+=-sSUPPORT_LONGJMP=wasm QMAKE_CXXFLAGS+=-sSUPPORT_LONGJMP=wasm
    make -j<CORES> module-qtbase

Note that `5.15.2+wasm` is a branch that is expected to contain further fixes as they become
necessary.

Do not include `-fwasm-exceptions` in the above `QMAKE_CXXFLAGS`, see
<https://emscripten.org/docs/api_reference/emscripten.h.html#c.emscripten_set_main_loop> "Note:
Currently, using the new Wasm exception handling and simulate_infinite_loop == true at the same time
does not work yet in C++ projects that have objects with destructors on the stack at the time of the
call."  (Also see the EMSCRIPTEN-specific HACK in soffice_main, desktop/source/app/sofficemain.cxx,
for what we need to do to work around that.)

Optionally you can add the configure flag "-compile-examples". But then you also have to
patch at least mkspecs/wasm-emscripten/qmake.conf with EXIT_RUNTIME=0, otherwise they will
fail to run. In addition, building with examples will break with some of them, but at that
point Qt already works and also most examples. Or just skip them. Other interesting flags
might be "-nomake tests -no-pch -ccache".

Linking takes quite a long time, because emscripten-finalize rewrites the whole WASM files with
some options. This way the LO WASM possibly needs 64GB RAM. For faster link times add
"-s WASM_BIGINT=1", change to ASSERTIONS=1 and use -g3 to prevent rewriting the WASM file and
generating source maps (see emscripten.py, finalize_wasm, and avoid modify_wasm = True). This is
just needed for Qt examples, as LO already uses the correct flags!

It's needed to install Qt5 to the chosen prefix. Else LO won't find all needed files in the
right place. For installation you can do

    make -j<CORES> install
or
    make -j8 -C qtbase/src install_subtargets

Current Qt fails to start the demo webserver: <https://bugreports.qt.io/browse/QTCREATORBUG-24072>

Use `emrun --serve_after_close` to run Qt WASM demos.

Qt builds some 3rd-party libraries that it brings along (e.g., qt5/qtbase/src/3rdparty/freetype) and
compiles its own code against the C/C++ include files of those 3rd-party libraries.  But when we
link LO, we link against our own versions of those libraries' archives (e.g.,
workdir/UnpackedTarball/freetype/instdir/lib/libfreetype.a), not against the Qt ones (e.g.,
$QT5DIR/lib/libqtfreetype.a).  This mismatch between the include files that Qt is compiled against,
vs. the archive actually linked in, seems to not cause issues in practice.  (If it did, we could
either try to make both Qt and LO link against e.g. -sUSE_FREETYPE from emscripten-ports, or we
could move Qt from a prerequisite to a proper external/qt5 LO module built during the LO build, and
hack its configuration to build against LO's external/freetype etc.  The former approach, building Qt
with -sUSE_FREETYPE, is even tried in qtbase/src/gui/configure.json, but apparently fails for
reasons not studied further yet, cf. Qt's config.log.)

### Setup LO

`autogen.sh` is patched to use emconfigure. That basically sets various
environment vars, especially `EMMAKEN_JUST_CONFIGURE`, which will create the
correct output file names, checked by `configure` (`a.out`).

There's a distro config for WASM, but it just provides --host=wasm32-local-emscripten, which
should be enough setup. The build itself is a cross build and the cross-toolset just depends
on a minimal toolset (gcc, libc-dev, flex, bison); all else is build from source, because the
final result is not depending on the build system at all.

Recommended configure setup is thusly:

* grab defaults
    `--with-distro=SnipeOfficeWASM32`

* local config
    `QT5DIR=/dir/of/qt5/install/prefix`

* if you want to use ccache on both sides of the build
```
--with-build-platform-configure-options=--enable-ccache
--enable-ccache
```

FWIW: it's also possible to build an almost static Linux SnipeOffice by just using
--disable-dynloading --enable-customtarget-components. System externals are still
linked dynamically, but everything else is static.

### "Deploying" soffice.wasm

```
tar -chf wasm.tar --xform 's/.*program/lo-wasm/' instdir/program/soffice.* \
    instdir/program/qt*
```

Your HTTP server needs to provide additional headers:
* add_header Cross-Origin-Opener-Policy same-origin
* add_header Cross-Origin-Embedder-Policy require-corp

The default html to use should be qt_soffice.html

### Debugging setup

Since a few months you can use DWARF information embedded by LLVM into the WASM
to debug WASM in Chrome. You need to enable an experimental feature and install
an additional extension. The whole setup is described in:

https://developer.chrome.com/blog/wasm-debugging-2020/

This way you don't need source maps (much faster linking!) and can resolve local
WASM variables to C++ names!

Per default, the WASM debug build splits the DWARF information into an additional
WASM file, postfixed '.debug.wasm'.

### Using Docker to cross-build with emscripten

If you prefer a controlled environment (sadly emsdk install/activate
is _not_ stable over time, as e.g. nodejs versions evolve), that is
easy to replicate across different machines - consider the docker
images we're providing.

Config/setup file see
<https://git.SnipeOffice.org/lode/+/ccb36979563635b51215477455953252c99ec013>

Run

```
docker-compose build
```

in the lode/docker dir to get the container prepared. Run

```
PARALLELISM=4 BUILD_OPTIONS= BUILD_TARGET=build docker-compose run --rm \
    -e PARALLELISM -e BUILD_TARGET -e BUILD_OPTIONS builder
```

to perform an actual `srcdir != builddir` build; the container mounts
checked-out git repo and output dir via `docker-compose.yml` (so make
sure the path names there match your setup):

The lode setup expects, inside the lode/docker subdir, the following directories:

- core (`git checkout`)
- workdir (the output dir - gets written into)
- cache (`ccache tree`)
- tarballs (external project tarballs gets written and cached there)

### UNO bindings with Embind

Right now there's a very rough implementation in place. With lots of different
bits unimplemented. And it _might_ be leaking memory. i.e. Lots of room for
improvement! ;)

Some usage examples through javascript of the current implementation:
```js
// inserts a string at the start of the Writer document.
Module.uno_init.then(function() {
    const css = Module.uno.com.sun.star;
    let xModel = Module.getCurrentModelFromViewSh();
    if (xModel === null || !css.text.XTextDocument.query(xModel)) {
        const desktop = css.frame.Desktop.create(Module.getUnoComponentContext());
        const args = new Module.uno_Sequence_com$sun$star$beans$PropertyValue(
            0, Module.uno_Sequence.FromSize);
        xModel = css.frame.XComponentLoader.query(desktop).loadComponentFromURL(
            'file:///android/default-document/example.odt', '_default', 0, args);
        args.delete();
    }
    const xTextDocument = css.text.XTextDocument.query(xModel);
    const xText = xTextDocument.getText();
    const xTextCursor = xText.createTextCursor();
    xTextCursor.setString("string here!");
});
```

```js
// changes each paragraph of the Writer document to a random color.
Module.uno_init.then(function() {
    const css = Module.uno.com.sun.star;
    let xModel = Module.getCurrentModelFromViewSh();
    if (xModel === null || !css.text.XTextDocument.query(xModel)) {
        const desktop = css.frame.Desktop.create(Module.getUnoComponentContext());
        const args = new Module.uno_Sequence_com$sun$star$beans$PropertyValue(
            0, Module.uno_Sequence.FromSize);
        xModel = css.frame.XComponentLoader.query(desktop).loadComponentFromURL(
            'file:///android/default-document/example.odt', '_default', 0, args);
        args.delete();
    }
    const xTextDocument = css.text.XTextDocument.query(xModel);
    const xText = xTextDocument.getText();
    const xEnumAccess = css.container.XEnumerationAccess.query(xText);
    const xParaEnumeration = xEnumAccess.createEnumeration();
    while (xParaEnumeration.hasMoreElements()) {
        const next = xParaEnumeration.nextElement();
        const xParagraph = css.text.XTextRange.query(next.get());
        const xParaProps = css.beans.XPropertySet.query(xParagraph);
        const color = new Module.uno_Any(
            Module.uno_Type.Long(), Math.floor(Math.random() * 0xFFFFFF));
        xParaProps.setPropertyValue("CharColor", color);
        next.delete();
        color.delete();
    }
});
```

If you enter the above examples into the browser console, you need to enter them into the console of
the first web worker thread, which is the LO main thread since we use -sPROXY_TO_PTHREAD, not
into the console of the browser's main thread.

Alternatively, you can do the following:  Put an example into some file like `example.js` that you
put next to the `qt_soffice.html` that you serve to the browser (i.e., in
`workdir/installation/SnipeOffice/emscripten/`).  Create another small JS snippet file like
`include.js` (which is only needed during the build) containing
```
Module.uno_scripts = ['./example.js'];
```
And rebuild LO configured with an additional
`EMSCRIPTEN_EXTRA_SOFFICE_PRE_JS=/...path-to.../include.js`.

## Tools for problem diagnosis

* `nm -s` should list the symbols in the archive, based on the index generated by ranlib.
  If you get linking errors that archive has no index.


## Emscripten filesystem access with threads

This is closed, but not really fixed IMHO:

- <https://github.com/emscripten-core/emscripten/issues/3922>

## Dynamic libraries `/` modules in emscripten

There is a good summary in:

- <https://bugreports.qt.io/browse/QTBUG-63925>

Summary: you can't use modules and threads.

This is mentioned at the end of:

- <https://github.com/emscripten-core/emscripten/wiki/Linking>

The usage of `MAIN_MODULE` and `SIDE_MODULE` has other problems, a major one IMHO is symbol resolution at runtime only.
So this works really more like plugins in the sense of symbol resolution without dependencies `/` rpath.

There is some clang-level dynamic-linking in progress (WASM dlload). The following link is already a bit old,
but I found it a god summary of problems to expect:

- <https://iandouglasscott.com/2019/07/18/experimenting-with-webassembly-dynamic-linking-with-clang/>


## Mixed information, links, problems, TODO

More info on Qt WASM emscripten pthreads:

- <https://wiki.qt.io/Qt_for_WebAssembly#Multithreading_Support>

WASM needs `-pthread` at compile, not just link time for atomics support. Alternatively you can provide
`-s USE_PTHREADS=1`, but both don't seem to work reliable, so best provide both.
<https://github.com/emscripten-core/emscripten/issues/10370>

The output file must have the prefix .o, otherwise the WASM files will get a
`node.js` shebang (!) and ranlib won't be able to index the library (link errors).

Qt with threads has further memory limit. From Qt configure:
```
Project MESSAGE: Setting PTHREAD_POOL_SIZE to 4
Project MESSAGE: Setting TOTAL_MEMORY to 1GB
```

You can actually allocate 4GB:

- <https://bugzilla.mozilla.org/show_bug.cgi?id=1392234>

LO uses a nested event loop to run dialogs in general, but that won't work, because you can't drive
the browser event loop. like VCL does with the system event loop in the various VCL backends.
Changing this will need some major work (basically dropping Application::Execute).

But with the know problems with exceptions and threads, this might change:

- <https://github.com/emscripten-core/emscripten/pull/11518>
- <https://github.com/emscripten-core/emscripten/issues/11503>
- <https://github.com/emscripten-core/emscripten/issues/11233>
- <https://github.com/emscripten-core/emscripten/issues/12035>

We're also using emconfigure at the moment. Originally I patched emscripten, because it
wouldn't create the correct a.out file for C++ configure tests. Later I found that
the `emconfigure` sets `EMMAKEN_JUST_CONFIGURE` to work around the problem.

ICU bug:

- <https://github.com/emscripten-core/emscripten/issues/10129>

Alternative, probably:

- <https://developer.mozilla.org/de/docs/Web/JavaScript/Reference/Global_Objects/Intl>

There is a wasm64, but that still uses 32bit pointers!

Old outdated docs:

- <https://wiki.SnipeOffice.org/Development/Emscripten>

Reverted patch:

- <https://cgit.freedesktop.org/SnipeOffice/core/commit/?id=0e21f6619c72f1e17a7b0a52b6317810973d8a3e>

Generally <https://emscripten.org/docs/porting>:

- <https://emscripten.org/docs/porting/guidelines/api_limitations.html#api-limitations>
- <https://emscripten.org/docs/porting/files/file_systems_overview.html#file-system-overview>
- <https://emscripten.org/docs/porting/pthreads.html>
- <https://emscripten.org/docs/porting/emscripten-runtime-environment.html>

This will be interesting:

- <https://emscripten.org/docs/getting_started/FAQ.html#how-do-i-run-an-event-loop>

This didn't help much yet:

- <https://github.com/emscripten-ports>

Emscripten supports standalone WASI binaries:

- <https://github.com/emscripten-core/emscripten/wiki/WebAssembly-Standalone>
- <https://www.qt.io/qt-examples-for-webassembly>
- <http://qtandeverything.blogspot.com/2017/06/qt-for-web-assembly.html>
- <http://qtandeverything.blogspot.com/2020/>
- <https://emscripten.org/docs/api_reference/Filesystem-API.html>
- <https://discuss.python.org/t/add-a-webassembly-wasm-runtime/3957/12>
- <http://git.savannah.gnu.org/cgit/config.git>
- <https://webassembly.org/specs/>
- <https://developer.chrome.com/docs/native-client/>
- <https://emscripten.org/docs/getting_started/downloads.html>
- <https://github.com/openpgpjs/openpgpjs/blob/master/README.md#getting-started>
- <https://developer.mozilla.org/en-US/docs/WebAssembly/Using_the_JavaScript_API>
- <https://github.com/bytecodealliance/wasmtime/blob/main/docs/WASI-intro.md>
- <https://www.ip6.li/de/security/x.509_kochbuch/openssl-fuer-webassembly-compilieren>
- <https://emscripten.org/docs/introducing_emscripten/about_emscripten.html#about-emscripten-porting-code>
- <https://emscripten.org/docs/compiling/Building-Projects.html>

### Threads and the event loop

The Emscripten emulation of pthreads requires the JS main thread event loop to be able to promptly
respond both when spawning and when exiting a pthread.  But the Qt5 event loop runs on the JS main
thread, so the JS main thread event loop is blocked while a LO VCL Task is executed.  And our
pthreads are typically spawned and joined from within such Task executions, which means that the JS
main thread event loop is not available to reliably perform those Emscripten pthread operations.

For pthread spawning, the solution is to set -sPTHREAD_POOL_SIZE to a sufficiently large value, so
that each of our pthread spawning requests during an inappropriate time finds a pre-spawned JS
Worker available.

There are patterns (like, at the time of writing this, the configmgr::Components::WriteThread) where
a pthread can get spawned and joined and then re-spawned (and re-joined) multiple times during a
single VCL Task execution (i.e., without the JS main thread event loop having a chance to get in
between any of those operations).  But as the underlying Emscripten pthread exiting operations will
therefore queue up, the pthread spawning operations will eventually run out of -sPTHREAD_POOL_SIZE
pre-spawned JS Workers.  The solution here is to change our pthread usage patterns accordingly, so
that such pthreads are rather kept running than being joined and re-spawned.

(-sPROXY_TO_PTHREAD would move the Qt5 event loop off the JS main thread, which should elegantly
solve all of the above issues.  But Qt5 just doesn't appear to be prepared to run on anything but
the JS main thread; e.g., it tries to access the global JS `window` object in various places, which
is available on the JS main thread but not in a JS Worker.)

## Building headless SnipeOffice as WASM for use in another product

### Set up Emscripten

Follow the instructions in the first part of this document.

### No Qt needed.

You don't need any dependencies other than those that normally are
downloaded and compiled when building SnipeOffice.

### Set up LO

For instance, this autogen.input works for me:

```
--disable-debug
--enable-sal-log
--disable-crashdump
--host=wasm32-local-emscripten
--disable-gui
--with-wasm-module=writer
--with-package-format=emscripten
```

For building LO core for use in COWASM, it is known to work to use
Emscripten 3.1.30 (and not just 2.0.31 which is what the LO+Qt5 work
has been using in the past).

### That's all

After all, in this case you are building LO core headless for it to be used by other software.

Note that a soffice.wasm will be built, but that is just because of
how the makefilery has been set up. We do need the soffice.data file
that contains the in-memory file system needed by the SnipeOffice
Technology core code during run-time, though. That is at the moment
built as a side-effect when building soffice.wasm.
