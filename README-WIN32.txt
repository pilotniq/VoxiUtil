
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

We use Cygwin for makefiles, configuration etc. but all compiling
and linking are done with MSVC command-line tools:

- "cl" is the C compiler (cc-equivalent)
- "link" is the linker (ld-equivalent)

The above should be detected automatically by autoconf/configure
et al. but can probably be forcefully set by setting the "CC"
and "LD" (or "LINK"?) environment variables.

In the source, the aim is now to try to avoid "OS-specific" macros as
much as possible (checking for "LINUX", "WIN32", etc). Instead, each
(potentially architecture-dependent) feature is checked for
individually and indicated with macros such as "HAVE_UNISTD_H",
"HAVE_MYSQL", etc. This gives a finer granularity when configuring
to specific local settings, as well as code that is easier to maintain.


aclocal
-------

It may be necessary to run aclocal like this:

- Install version 1.7

- Make sure libtool.m4 is in the share/aclocal-1.7 of the installation
  directory (typically /usr/local/share/aclocal-1.7):

cp /usr/autotool/stable/share/aclocal/libtool.m4 /usr/local/share/aclocal-1.7cp /usr/autotool/

- Run aclocal like this:

aclocal -I /usr/local/share/aclocal-1.7


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


