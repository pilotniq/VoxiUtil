
Notes on configuring voxi source on Windows
===========================================

  Mårten Stenius, mst@voxi.se, November, December 2002


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

In the Voxi source, the aim is now to try to avoid "OS-specific"
macros as much as possible (checking for "LINUX", "WIN32",
etc). Instead, each (potentially architecture-dependent) feature is
checked for individually and indicated with macros such as
"HAVE_UNISTD_H", "HAVE_MYSQL", etc. This gives a finer granularity
when configuring to specific local settings, as well as code that is
easier to maintain.

However, in some cases we still need to use the WIN32 macro,
such as in DLL configuration (see that section)


DLL configuration
-----------------

(NOTE: Unfortunately we have to circumvent the use of libtool on
Windows platforms. For a discussion on why, see the section "Why not
libtool on Win32?" below).

To correctly produce Win32 DLL:s correctly we must obey the rather
clunky rules for DLL exports and imports, which differ significantly
from the procedures employed on linux. This is one of many things that
cannot be completely automatically done on Win32 through libtool.

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

2) In a manually coded .def file for each module

This would mean listing each function with an ordinal in a separate
text file ("foo.def"). A major disadvantage is that you must maintain
an extra file for the API definitions, which is bound to be a pain.
One advantage would be that you would not need the macros described
in the previous sections (but they are only defined once and
are thus easier to maintain in the long run).

Nevertheless, with the Windows CE port it turns out that we might have
to start coding DEF files manually anyway. We currently generate them
automatically on Win32 but this does not work when compiling for
WinCE, since generation tool we use (dlltool from cygwin) does not
work with such binaries. And, since the exported set of symbol can
differ between platforms, we cannot just use the same automatically
generated .def files over all Windows related platforms...


Why not libtool on Win32?
-------------------------

Yes, why not? The mission of libtool, according to its home
page, is "to support an arbitrary number of library object types"
and allow the user to write platform-independent rules for
building libraries, shared or static. Sounds nice. In theory.

So today libtool would, if it also had the ambition to support the
major OS platforms currently in use, have full and transparent support
for the Win32 platform. Now, after much struggling and digging into
libtool source, manuals, and mailing lists I have come to the
conclusion that the support for building DLL:s is not sufficient.
Since it is an open-source project, we could some day look into this
ourselves and contribute to the project. However, this is a
significant effort that is bound to be larger than circumventing
libtool for the time being, unfortunately. This refers to the
situation as I perceive it in December 2002.

Getting libtool to build a static library with the MSVC tools was no
big problem and worked pretty much "out of the box". But DLL:s are a
different matter. Several posts on the libtool mailing list
(libtool@gnu.org) confirms my experience here.

For starters, the AC_LIBTOOL_WIN32_DLL macro apparently does not
affect the decision on whether to build DLL:s or not, as pointed
out in a post dated 26 Oct 2002. And, furthermore, the test that
is finally applied is in no way adopted to the MSVC linker.

Also, the rather peculiar process of creating DLL:s on Win32,
done with export/import definitions, is not fully supported.
People have mentioned that, to do this with MSVC, they have
resorted to "hacks". Not good.

Now, I have momentarily (re)considered trying gcc with the mingw
toolkit but seemingly, things start to become really difficult there
too, as soon as you want to build DLL:s. And furthermore, with
gcc/mingw we would not be using the OS-native compilers on Win32
anymore - a bad principle for long-term compatibility on any platform
IMHO, and definitely on Win32.

The bottom line is: As of December 2002, libtool fails in fully
mapping down to the MSVC linking tools and is unfortunately too
tuned towards the "Unix" style of creating libraries. The effort of
getting libtool to work in this respect (with hacks, contributing to
libtool development, whatever is needed) is likely to be larger for us
than just creating custom rules in Makefile.am for the Win32 DLL:s.


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


