/**
 * @file loglevel.h
 * @author Ambroz Bizjak <ambrop7@gmail.com>
 * 
 * @section LICENSE
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * @section DESCRIPTION
 * 
 * Log level specification parsing function.
 */

#ifndef BADVPN_MISC_LOGLEVEL_H
#define BADVPN_MISC_LOGLEVEL_H

#include <string.h>

#include <base/BLog.h>

/**
 * Parses the log level string.
 * 
 * @param str log level string. Recognizes none, error, warning, notice,
 *            info, debug.
 * @return 0 for none, one of BLOG_* for some log level, -1 for unrecognized
 */
static int parse_loglevel (char *str);

int parse_loglevel (char *str)
{
    if (!strcmp(str, "none")) {
        return 0;
    }
    if (!strcmp(str, "error")) {
        return BLOG_ERROR;
    }
    if (!strcmp(str, "warning")) {
        return BLOG_WARNING;
    }
    if (!strcmp(str, "notice")) {
        return BLOG_NOTICE;
    }
    if (!strcmp(str, "info")) {
        return BLOG_INFO;
    }
    if (!strcmp(str, "debug")) {
        return BLOG_DEBUG;
    }
    
    char *endptr;
    long int res = strtol(str, &endptr, 10);
    if (*str && !*endptr && res >= 0 && res <= BLOG_DEBUG) {
        return res;
    }
    
    return -1;
}

#endif
