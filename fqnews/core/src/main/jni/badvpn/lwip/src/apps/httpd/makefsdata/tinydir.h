/*
Copyright (c) 2013-2017, tinydir authors:
- Cong Xu
- Lautis Sun
- Baudouin Feildel
- Andargor <andargor@yahoo.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef TINYDIR_H
#define TINYDIR_H

#ifdef __cplusplus
extern "C" {
#endif

#if ((defined _UNICODE) && !(defined UNICODE))
#define UNICODE
#endif

#if ((defined UNICODE) && !(defined _UNICODE))
#define _UNICODE
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <tchar.h>
# pragma warning(push)
# pragma warning (disable : 4996)
#else
# include <dirent.h>
# include <libgen.h>
# include <sys/stat.h>
# include <stddef.h>
#endif
#ifdef __MINGW32__
# include <tchar.h>
#endif


/* types */

/* Windows UNICODE wide character support */
#if defined _MSC_VER || defined __MINGW32__
#define _tinydir_char_t TCHAR
#define TINYDIR_STRING(s) _TEXT(s)
#define _tinydir_strlen _tcslen
#define _tinydir_strcpy _tcscpy
#define _tinydir_strcat _tcscat
#define _tinydir_strcmp _tcscmp
#define _tinydir_strrchr _tcsrchr
#define _tinydir_strncmp _tcsncmp
#else
#define _tinydir_char_t char
#define TINYDIR_STRING(s) s
#define _tinydir_strlen strlen
#define _tinydir_strcpy strcpy
#define _tinydir_strcat strcat
#define _tinydir_strcmp strcmp
#define _tinydir_strrchr strrchr
#define _tinydir_strncmp strncmp
#endif

#if (defined _MSC_VER || defined __MINGW32__)
#include <windows.h>
#define _TINYDIR_PATH_MAX MAX_PATH
#elif defined  __linux__
#include <linux/limits.h>
#define _TINYDIR_PATH_MAX PATH_MAX
#else
#define _TINYDIR_PATH_MAX 4096
#endif

#ifdef _MSC_VER
/* extra chars for the "\\*" mask */
# define _TINYDIR_PATH_EXTRA 2
#else
# define _TINYDIR_PATH_EXTRA 0
#endif

#define _TINYDIR_FILENAME_MAX 256

#if (defined _MSC_VER || defined __MINGW32__)
#define _TINYDIR_DRIVE_MAX 3
#endif

#ifdef _MSC_VER
# define _TINYDIR_FUNC static __inline
#elif !defined __STDC_VERSION__ || __STDC_VERSION__ < 199901L
# define _TINYDIR_FUNC static __inline__
#else
# define _TINYDIR_FUNC static inline
#endif

/* readdir_r usage; define TINYDIR_USE_READDIR_R to use it (if supported) */
#ifdef TINYDIR_USE_READDIR_R

/* readdir_r is a POSIX-only function, and may not be available under various
 * environments/settings, e.g. MinGW. Use readdir fallback */
#if _POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _BSD_SOURCE || _SVID_SOURCE ||\
	_POSIX_SOURCE
# define _TINYDIR_HAS_READDIR_R
#endif
#if _POSIX_C_SOURCE >= 200112L
# define _TINYDIR_HAS_FPATHCONF
# include <unistd.h>
#endif
#if _BSD_SOURCE || _SVID_SOURCE || \
	(_POSIX_C_SOURCE >= 200809L || _XOPEN_SOURCE >= 700)
# define _TINYDIR_HAS_DIRFD
# include <sys/types.h>
#endif
#if defined _TINYDIR_HAS_FPATHCONF && defined _TINYDIR_HAS_DIRFD &&\
	defined _PC_NAME_MAX
# define _TINYDIR_USE_FPATHCONF
#endif
#if defined __MINGW32__ || !defined _TINYDIR_HAS_READDIR_R ||\
	!(defined _TINYDIR_USE_FPATHCONF || defined NAME_MAX)
# define _TINYDIR_USE_READDIR
#endif

/* Use readdir by default */
#else
# define _TINYDIR_USE_READDIR
#endif

/* MINGW32 has two versions of dirent, ASCII and UNICODE*/
#ifndef _MSC_VER
#if (defined __MINGW32__) && (defined _UNICODE)
#define _TINYDIR_DIR _WDIR
#define _tinydir_dirent _wdirent
#define _tinydir_opendir _wopendir
#define _tinydir_readdir _wreaddir
#define _tinydir_closedir _wclosedir
#else
#define _TINYDIR_DIR DIR
#define _tinydir_dirent dirent
#define _tinydir_opendir opendir
#define _tinydir_readdir readdir
#define _tinydir_closedir closedir
#endif
#endif

/* Allow user to use a custom allocator by defining _TINYDIR_MALLOC and _TINYDIR_FREE. */
#if    defined(_TINYDIR_MALLOC) &&  defined(_TINYDIR_FREE)
#elif !defined(_TINYDIR_MALLOC) && !defined(_TINYDIR_FREE)
#else
#error "Either define both alloc and free or none of them!"
#endif

#if !defined(_TINYDIR_MALLOC)
	#define _TINYDIR_MALLOC(_size) malloc(_size)
	#define _TINYDIR_FREE(_ptr)    free(_ptr)
#endif /* !defined(_TINYDIR_MALLOC) */

typedef struct tinydir_file
{
	_tinydir_char_t path[_TINYDIR_PATH_MAX];
	_tinydir_char_t name[_TINYDIR_FILENAME_MAX];
	_tinydir_char_t *extension;
	int is_dir;
	int is_reg;

#ifndef _MSC_VER
#ifdef __MINGW32__
	struct _stat _s;
#else
	struct stat _s;
#endif
#endif
} tinydir_file;

typedef struct tinydir_dir
{
	_tinydir_char_t path[_TINYDIR_PATH_MAX];
	int has_next;
	size_t n_files;

	tinydir_file *_files;
#ifdef _MSC_VER
	HANDLE _h;
	WIN32_FIND_DATA _f;
#else
	_TINYDIR_DIR *_d;
	struct _tinydir_dirent *_e;
#ifndef _TINYDIR_USE_READDIR
	struct _tinydir_dirent *_ep;
#endif
#endif
} tinydir_dir;


/* declarations */

_TINYDIR_FUNC
int tinydir_open(tinydir_dir *dir, const _tinydir_char_t *path);
_TINYDIR_FUNC
int tinydir_open_sorted(tinydir_dir *dir, const _tinydir_char_t *path);
_TINYDIR_FUNC
void tinydir_close(tinydir_dir *dir);

_TINYDIR_FUNC
int tinydir_next(tinydir_dir *dir);
_TINYDIR_FUNC
int tinydir_readfile(const tinydir_dir *dir, tinydir_file *file);
_TINYDIR_FUNC
int tinydir_readfile_n(const tinydir_dir *dir, tinydir_file *file, size_t i);
_TINYDIR_FUNC
int tinydir_open_subdir_n(tinydir_dir *dir, size_t i);

_TINYDIR_FUNC
int tinydir_file_open(tinydir_file *file, const _tinydir_char_t *path);
_TINYDIR_FUNC
void _tinydir_get_ext(tinydir_file *file);
_TINYDIR_FUNC
int _tinydir_file_cmp(const void *a, const void *b);
#ifndef _MSC_VER
#ifndef _TINYDIR_USE_READDIR
_TINYDIR_FUNC
size_t _tinydir_dirent_buf_size(_TINYDIR_DIR *dirp);
#endif
#endif


/* definitions*/

_TINYDIR_FUNC
int tinydir_open(tinydir_dir *dir, const _tinydir_char_t *path)
{
#ifndef _MSC_VER
#ifndef _TINYDIR_USE_READDIR
	int error;
	int size;	/* using int size */
#endif
#else
	_tinydir_char_t path_buf[_TINYDIR_PATH_MAX];
#endif
	_tinydir_char_t *pathp;

	if (dir == NULL || path == NULL || _tinydir_strlen(path) == 0)
	{
		errno = EINVAL;
		return -1;
	}
	if (_tinydir_strlen(path) + _TINYDIR_PATH_EXTRA >= _TINYDIR_PATH_MAX)
	{
		errno = ENAMETOOLONG;
		return -1;
	}

	/* initialise dir */
	dir->_files = NULL;
#ifdef _MSC_VER
	dir->_h = INVALID_HANDLE_VALUE;
#else
	dir->_d = NULL;
#ifndef _TINYDIR_USE_READDIR
	dir->_ep = NULL;
#endif
#endif
	tinydir_close(dir);

	_tinydir_strcpy(dir->path, path);
	/* Remove trailing slashes */
	pathp = &dir->path[_tinydir_strlen(dir->path) - 1];
	while (pathp != dir->path && (*pathp == TINYDIR_STRING('\\') || *pathp == TINYDIR_STRING('/')))
	{
		*pathp = TINYDIR_STRING('\0');
		pathp++;
	}
#ifdef _MSC_VER
	_tinydir_strcpy(path_buf, dir->path);
	_tinydir_strcat(path_buf, TINYDIR_STRING("\\*"));
#if (defined WINAPI_FAMILY) && (WINAPI_FAMILY != WINAPI_FAMILY_DESKTOP_APP)
	dir->_h = FindFirstFileEx(path_buf, FindExInfoStandard, &dir->_f, FindExSearchNameMatch, NULL, 0);
#else
	dir->_h = FindFirstFile(path_buf, &dir->_f);
#endif
	if (dir->_h == INVALID_HANDLE_VALUE)
	{
		errno = ENOENT;
#else
	dir->_d = _tinydir_opendir(path);
	if (dir->_d == NULL)
	{
#endif
		goto bail;
	}

	/* read first file */
	dir->has_next = 1;
#ifndef _MSC_VER
#ifdef _TINYDIR_USE_READDIR
	dir->_e = _tinydir_readdir(dir->_d);
#else
	/* allocate dirent buffer for readdir_r */
	size = _tinydir_dirent_buf_size(dir->_d); /* conversion to int */
	if (size == -1) return -1;
	dir->_ep = (struct _tinydir_dirent*)_TINYDIR_MALLOC(size);
	if (dir->_ep == NULL) return -1;

	error = readdir_r(dir->_d, dir->_ep, &dir->_e);
	if (error != 0) return -1;
#endif
	if (dir->_e == NULL)
	{
		dir->has_next = 0;
	}
#endif

	return 0;

bail:
	tinydir_close(dir);
	return -1;
}

_TINYDIR_FUNC
int tinydir_open_sorted(tinydir_dir *dir, const _tinydir_char_t *path)
{
	/* Count the number of files first, to pre-allocate the files array */
	size_t n_files = 0;
	if (tinydir_open(dir, path) == -1)
	{
		return -1;
	}
	while (dir->has_next)
	{
		n_files++;
		if (tinydir_next(dir) == -1)
		{
			goto bail;
		}
	}
	tinydir_close(dir);

	if (tinydir_open(dir, path) == -1)
	{
		return -1;
	}

	dir->n_files = 0;
	dir->_files = (tinydir_file *)_TINYDIR_MALLOC(sizeof *dir->_files * n_files);
	if (dir->_files == NULL)
	{
		goto bail;
	}
	while (dir->has_next)
	{
		tinydir_file *p_file;
		dir->n_files++;

		p_file = &dir->_files[dir->n_files - 1];
		if (tinydir_readfile(dir, p_file) == -1)
		{
			goto bail;
		}

		if (tinydir_next(dir) == -1)
		{
			goto bail;
		}

		/* Just in case the number of files has changed between the first and
		second reads, terminate without writing into unallocated memory */
		if (dir->n_files == n_files)
		{
			break;
		}
	}

	qsort(dir->_files, dir->n_files, sizeof(tinydir_file), _tinydir_file_cmp);

	return 0;

bail:
	tinydir_close(dir);
	return -1;
}

_TINYDIR_FUNC
void tinydir_close(tinydir_dir *dir)
{
	if (dir == NULL)
	{
		return;
	}

	memset(dir->path, 0, sizeof(dir->path));
	dir->has_next = 0;
	dir->n_files = 0;
	_TINYDIR_FREE(dir->_files);
	dir->_files = NULL;
#ifdef _MSC_VER
	if (dir->_h != INVALID_HANDLE_VALUE)
	{
		FindClose(dir->_h);
	}
	dir->_h = INVALID_HANDLE_VALUE;
#else
	if (dir->_d)
	{
		_tinydir_closedir(dir->_d);
	}
	dir->_d = NULL;
	dir->_e = NULL;
#ifndef _TINYDIR_USE_READDIR
	_TINYDIR_FREE(dir->_ep);
	dir->_ep = NULL;
#endif
#endif
}

_TINYDIR_FUNC
int tinydir_next(tinydir_dir *dir)
{
	if (dir == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	if (!dir->has_next)
	{
		errno = ENOENT;
		return -1;
	}

#ifdef _MSC_VER
	if (FindNextFile(dir->_h, &dir->_f) == 0)
#else
#ifdef _TINYDIR_USE_READDIR
	dir->_e = _tinydir_readdir(dir->_d);
#else
	if (dir->_ep == NULL)
	{
		return -1;
	}
	if (readdir_r(dir->_d, dir->_ep, &dir->_e) != 0)
	{
		return -1;
	}
#endif
	if (dir->_e == NULL)
#endif
	{
		dir->has_next = 0;
#ifdef _MSC_VER
		if (GetLastError() != ERROR_SUCCESS &&
			GetLastError() != ERROR_NO_MORE_FILES)
		{
			tinydir_close(dir);
			errno = EIO;
			return -1;
		}
#endif
	}

	return 0;
}

_TINYDIR_FUNC
int tinydir_readfile(const tinydir_dir *dir, tinydir_file *file)
{
	if (dir == NULL || file == NULL)
	{
		errno = EINVAL;
		return -1;
	}
#ifdef _MSC_VER
	if (dir->_h == INVALID_HANDLE_VALUE)
#else
	if (dir->_e == NULL)
#endif
	{
		errno = ENOENT;
		return -1;
	}
	if (_tinydir_strlen(dir->path) +
		_tinydir_strlen(
#ifdef _MSC_VER
			dir->_f.cFileName
#else
			dir->_e->d_name
#endif
		) + 1 + _TINYDIR_PATH_EXTRA >=
		_TINYDIR_PATH_MAX)
	{
		/* the path for the file will be too long */
		errno = ENAMETOOLONG;
		return -1;
	}
	if (_tinydir_strlen(
#ifdef _MSC_VER
			dir->_f.cFileName
#else
			dir->_e->d_name
#endif
		) >= _TINYDIR_FILENAME_MAX)
	{
		errno = ENAMETOOLONG;
		return -1;
	}

	_tinydir_strcpy(file->path, dir->path);
	_tinydir_strcat(file->path, TINYDIR_STRING("/"));
	_tinydir_strcpy(file->name,
#ifdef _MSC_VER
		dir->_f.cFileName
#else
		dir->_e->d_name
#endif
	);
	_tinydir_strcat(file->path, file->name);
#ifndef _MSC_VER
#ifdef __MINGW32__
	if (_tstat(
#else
	if (stat(
#endif
		file->path, &file->_s) == -1)
	{
		return -1;
	}
#endif
	_tinydir_get_ext(file);

	file->is_dir =
#ifdef _MSC_VER
		!!(dir->_f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
#else
		S_ISDIR(file->_s.st_mode);
#endif
	file->is_reg =
#ifdef _MSC_VER
		!!(dir->_f.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) ||
		(
			!(dir->_f.dwFileAttributes & FILE_ATTRIBUTE_DEVICE) &&
			!(dir->_f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			!(dir->_f.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) &&
#ifdef FILE_ATTRIBUTE_INTEGRITY_STREAM
			!(dir->_f.dwFileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM) &&
#endif
#ifdef FILE_ATTRIBUTE_NO_SCRUB_DATA
			!(dir->_f.dwFileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA) &&
#endif
			!(dir->_f.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) &&
			!(dir->_f.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY));
#else
		S_ISREG(file->_s.st_mode);
#endif

	return 0;
}

_TINYDIR_FUNC
int tinydir_readfile_n(const tinydir_dir *dir, tinydir_file *file, size_t i)
{
	if (dir == NULL || file == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	if (i >= dir->n_files)
	{
		errno = ENOENT;
		return -1;
	}

	memcpy(file, &dir->_files[i], sizeof(tinydir_file));
	_tinydir_get_ext(file);

	return 0;
}

_TINYDIR_FUNC
int tinydir_open_subdir_n(tinydir_dir *dir, size_t i)
{
	_tinydir_char_t path[_TINYDIR_PATH_MAX];
	if (dir == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	if (i >= dir->n_files || !dir->_files[i].is_dir)
	{
		errno = ENOENT;
		return -1;
	}

	_tinydir_strcpy(path, dir->_files[i].path);
	tinydir_close(dir);
	if (tinydir_open_sorted(dir, path) == -1)
	{
		return -1;
	}

	return 0;
}

/* Open a single file given its path */
_TINYDIR_FUNC
int tinydir_file_open(tinydir_file *file, const _tinydir_char_t *path)
{
	tinydir_dir dir;
	int result = 0;
	int found = 0;
	_tinydir_char_t dir_name_buf[_TINYDIR_PATH_MAX];
	_tinydir_char_t file_name_buf[_TINYDIR_FILENAME_MAX];
	_tinydir_char_t *dir_name;
	_tinydir_char_t *base_name;
#if (defined _MSC_VER || defined __MINGW32__)
	_tinydir_char_t drive_buf[_TINYDIR_PATH_MAX];
	_tinydir_char_t ext_buf[_TINYDIR_FILENAME_MAX];
#endif

	if (file == NULL || path == NULL || _tinydir_strlen(path) == 0)
	{
		errno = EINVAL;
		return -1;
	}
	if (_tinydir_strlen(path) + _TINYDIR_PATH_EXTRA >= _TINYDIR_PATH_MAX)
	{
		errno = ENAMETOOLONG;
		return -1;
	}

	/* Get the parent path */
#if (defined _MSC_VER || defined __MINGW32__)
#if ((defined _MSC_VER) && (_MSC_VER >= 1400))
		_tsplitpath_s(
			path,
			drive_buf, _TINYDIR_DRIVE_MAX,
			dir_name_buf, _TINYDIR_FILENAME_MAX,
			file_name_buf, _TINYDIR_FILENAME_MAX,
			ext_buf, _TINYDIR_FILENAME_MAX);
#else
		_tsplitpath(
			path,
			drive_buf,
			dir_name_buf,
			file_name_buf,
			ext_buf);
#endif

/* _splitpath_s not work fine with only filename and widechar support */
#ifdef _UNICODE
		if (drive_buf[0] == L'\xFEFE')
			drive_buf[0] = '\0';
		if (dir_name_buf[0] == L'\xFEFE')
			dir_name_buf[0] = '\0';
#endif

	if (errno)
	{
		errno = EINVAL;
		return -1;
	}
	/* Emulate the behavior of dirname by returning "." for dir name if it's
	empty */
	if (drive_buf[0] == '\0' && dir_name_buf[0] == '\0')
	{
		_tinydir_strcpy(dir_name_buf, TINYDIR_STRING("."));
	}
	/* Concatenate the drive letter and dir name to form full dir name */
	_tinydir_strcat(drive_buf, dir_name_buf);
	dir_name = drive_buf;
	/* Concatenate the file name and extension to form base name */
	_tinydir_strcat(file_name_buf, ext_buf);
	base_name = file_name_buf;
#else
	_tinydir_strcpy(dir_name_buf, path);
	dir_name = dirname(dir_name_buf);
	_tinydir_strcpy(file_name_buf, path);
	base_name =basename(file_name_buf);
#endif

	/* Open the parent directory */
	if (tinydir_open(&dir, dir_name) == -1)
	{
		return -1;
	}

	/* Read through the parent directory and look for the file */
	while (dir.has_next)
	{
		if (tinydir_readfile(&dir, file) == -1)
		{
			result = -1;
			goto bail;
		}
		if (_tinydir_strcmp(file->name, base_name) == 0)
		{
			/* File found */
			found = 1;
			break;
		}
		tinydir_next(&dir);
	}
	if (!found)
	{
		result = -1;
		errno = ENOENT;
	}

bail:
	tinydir_close(&dir);
	return result;
}

_TINYDIR_FUNC
void _tinydir_get_ext(tinydir_file *file)
{
	_tinydir_char_t *period = _tinydir_strrchr(file->name, TINYDIR_STRING('.'));
	if (period == NULL)
	{
		file->extension = &(file->name[_tinydir_strlen(file->name)]);
	}
	else
	{
		file->extension = period + 1;
	}
}

_TINYDIR_FUNC
int _tinydir_file_cmp(const void *a, const void *b)
{
	const tinydir_file *fa = (const tinydir_file *)a;
	const tinydir_file *fb = (const tinydir_file *)b;
	if (fa->is_dir != fb->is_dir)
	{
		return -(fa->is_dir - fb->is_dir);
	}
	return _tinydir_strncmp(fa->name, fb->name, _TINYDIR_FILENAME_MAX);
}

#ifndef _MSC_VER
#ifndef _TINYDIR_USE_READDIR
/*
The following authored by Ben Hutchings <ben@decadent.org.uk>
from https://womble.decadent.org.uk/readdir_r-advisory.html
*/
/* Calculate the required buffer size (in bytes) for directory      *
* entries read from the given directory handle.  Return -1 if this  *
* this cannot be done.                                              *
*                                                                   *
* This code does not trust values of NAME_MAX that are less than    *
* 255, since some systems (including at least HP-UX) incorrectly    *
* define it to be a smaller value.                                  */
_TINYDIR_FUNC
size_t _tinydir_dirent_buf_size(_TINYDIR_DIR *dirp)
{
	long name_max;
	size_t name_end;
	/* parameter may be unused */
	(void)dirp;

#if defined _TINYDIR_USE_FPATHCONF
	name_max = fpathconf(dirfd(dirp), _PC_NAME_MAX);
	if (name_max == -1)
#if defined(NAME_MAX)
		name_max = (NAME_MAX > 255) ? NAME_MAX : 255;
#else
		return (size_t)(-1);
#endif
#elif defined(NAME_MAX)
 	name_max = (NAME_MAX > 255) ? NAME_MAX : 255;
#else
#error "buffer size for readdir_r cannot be determined"
#endif
	name_end = (size_t)offsetof(struct _tinydir_dirent, d_name) + name_max + 1;
	return (name_end > sizeof(struct _tinydir_dirent) ?
		name_end : sizeof(struct _tinydir_dirent));
}
#endif
#endif

#ifdef __cplusplus
}
#endif

# if defined (_MSC_VER)
# pragma warning(pop)
# endif

#endif
