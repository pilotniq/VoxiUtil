
Notes on configuring voxi source on Windows
===========================================

  mst@voxi, November, December 2002


Intro
-----

This is a somewhat random list of things to think about when you set
about to configure voxi source on Windows. The current scope of this
document is modern NT-based Win32 platforms (XP, 2000, etc) but can be
expected to be extended to cover WinCE (PocketPC) in the not-too-distant
future.


Compilation tools
-----------------

We use Cygwin for makefiles, configuration etc. but all compiling
and linking are done with MSVC command-line tools:

- "cl" is the C compiler (cc-equivalent)
- "link" is the linker (ld-equivalent)

The above should be detected automatically by autoconf/configure
et al. but can probably be forcefully set by setting the "CC"
and "LD" (or "LINK"?) environment variables.


The WIN32 macro
---------------

In the source, the aim is now to try to avoid "OS-specific" macros as
much as possible (checking for "LINUX", "WIN32", etc). Instead, each
(potentially architecture-dependent) feature is checked for
individually and indicated with macros such as "HAVE_UNISTD_H",
"HAVE_MYSQL", etc. This gives a finer granularity when configuring
to specific local settings, as well as code that is easier to
maintain.

However, in some cases we still need to use the WIN32 macro,
such as in DLL configuration (see that section)


DLL configuration
-----------------

Libtool will then take care of certain parts of DLL constructions on
Win32: Using the correct linker etc (running link.exe instead of ld
etc) and then build the DLL:s. See libtool docs for details.

However, to correctly produce Win32 DLL:s correctly we must obey the
rather clunky rules for DLL exports and imports, which differ
significantly from the procedures employed on linux. This cannot be
completely automatically done through libtool/autoconf. Rather,
libtool expects us to handle export and import declarations by
ourselves as a pre-requisite to using the automatic macros in the
configure scripts.

Now, there are two ways of defining the exported symbols on Win32:

1) With Win32-specific modifiers in the header files of the module

This is what we currently use, and has the advantage of keeping
the export definitions in the same place as the function prototypes
itself which makes code maintenance easier.

One disadvantage of this approach is that the C symbols must use
export/import modifiers differently depending on whether the
declarations are included from a "user" of a library, or from the
building of that library itself. Thus we must have a way of detecting
who is including a particular .h file when compiling. For this
purpose, in the different source modules, we check for a macro like
"LIB_FOO_INTERNAL" to be defined if it is the module "foo" itself that
is being compiled.

Also, this whole construction means that we in the source code still
must know that we are compiling with Win32 tools, and thus we need to
use the WIN32 macro too.

Confusing? See util/threading.h for an example.

2) In a separate .def file for each module

This would mean listing each function with an ordinal in a separate
text file ("foo.def"). A major disadvantage is that you must maintain
an extra file for the API definitions, which is bound to be a pain.
One advantage would be that you would not need the macros described
in the previous sections (but they are only defined once and
are thus easier to maintain in the long run).

Nevertheless, with the Windows CE port it turns out that we might have
to start coding DEF files directly anyway. We currently generate them
automatically on Win32 but this does not work when compiling for
WinCE, since generation tool we use, dlltool from cygwin, does not
work with such binaries. And, since the exported set of symbol
can differ between platforms, we cannot just use the same .def
files over all Windows related platforms...



aclocal
-------

It may be necessary to run aclocal like this:

- Install version 1.7

- Make sure libtool.m4 is in the share/aclocal-1.7 of the installation
  directory (typically /usr/local/share/aclocal-1.7):

cp /usr/autotool/stable/share/aclocal/libtool.m4 /usr/local/share/aclocal-1.7cp /usr/autotool/

- If aclocal complains that some libtool related macro does not exist,
either run aclocal like this:

   aclocal -I /usr/local/share/aclocal-1.7

...or make sure that libtool.m4 is in the share/aclocal directory
directly.


Libtool
-------

- Libtool uses the environment variable SED. If you get a strange
error like "../libtool: s%^.*/%%: No such file or directory",
you need to define SED somewhere to:

SED=sed


POSIX threads
-------------

We currently use (and contribute to) pthreads-win32 for wrapping the
Win32 thread interface into a standard-looking pthread interface.

- You must make sure the position of the header files are
incorporated in your INCLUDE environment variable, otherwise
autoconf/configure won't detect them.


