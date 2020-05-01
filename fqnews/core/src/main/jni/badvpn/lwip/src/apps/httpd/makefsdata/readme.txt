This directory contains a script ('makefsdata') to create C code suitable for
httpd for given html pages (or other files) in a directory.

There is also a plain C console application doing the same and extended a bit.

Usage: htmlgen [targetdir] [-s] [-i]s
   targetdir: relative or absolute path to files to convert
   switch -s: toggle processing of subdirectories (default is on)
   switch -e: exclude HTTP header from file (header is created at runtime, default is on)
   switch -11: include HTTP 1.1 header (1.0 is default)

  if targetdir not specified, makefsdata will attempt to
  process files in subdirectory 'fs'.
