noinst_LTLIBRARIES = libcork.la

cli_src = src/libcork/cli/commands.c
core_src = src/libcork/core/allocator.c src/libcork/core/error.c src/libcork/core/gc.c \
		   src/libcork/core/hash.c src/libcork/core/ip-address.c src/libcork/core/mempool.c \
		   src/libcork/core/timestamp.c src/libcork/core/u128.c
ds_src = src/libcork/ds/array.c src/libcork/ds/bitset.c src/libcork/ds/buffer.c src/libcork/ds/dllist.c \
		 src/libcork/ds/file-stream.c src/libcork/ds/hash-table.c src/libcork/ds/managed-buffer.c \
		 src/libcork/ds/ring-buffer.c src/libcork/ds/slice.c
posix_src = src/libcork/posix/directory-walker.c src/libcork/posix/env.c src/libcork/posix/exec.c \
			src/libcork/posix/files.c src/libcork/posix/process.c
if !BUILD_WINCOMPAT
posix_src += src/libcork/posix/subprocess.c src/libcork/pthreads/thread.c
else
posix_src += src/libcork/posix/mingw.c
endif

libcork_la_SOURCES = $(cli_src) $(core_src) $(ds_src) $(pthreads_src) $(posix_src)
libcork_la_CFLAGS = -I$(top_srcdir)/libcork/include -DCORK_API=CORK_LOCAL
					 
libcork_la_LDFLAGS = -static
