/*
Copyright (c) 2004-2006 by Juliusz Chroboczek

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "polipo.h"

int
buildClientAuthHeaders(AtomPtr url, char *word,
                       AtomPtr *message_return, AtomPtr *headers_return)
{
    int code;
    char *h;
    AtomPtr message, headers;
    if(urlIsLocal(url->string, url->length)) {
        code = 401;
        message = internAtomF("Server authentication %s", word);
        h = "WWW-Authenticate";
    } else {
        code = 407;
        message = internAtomF("Proxy authentication %s", word);
        h = "Proxy-Authenticate";
    }
    headers = internAtomF("\r\n%s: Basic realm=\"%s\"",
                          h, authRealm->string);
    if(message_return)
        *message_return = message;
    else
        releaseAtom(message);
    *headers_return = headers;
    return code;
}

int
checkClientAuth(AtomPtr auth, AtomPtr url,
                AtomPtr *message_return, AtomPtr *headers_return)
{
    int code = 0;
    AtomPtr message = NULL, headers = NULL;

    if(authRealm == NULL || authCredentials == NULL)
        return 0;

    if(auth == NULL)
        code = buildClientAuthHeaders(url, "required", &message, &headers);
    else if(auth->length >= 6 || lwrcmp(auth->string, "basic ", 6) == 0) {
        if(b64cmp(auth->string + 6, auth->length - 6,
                  authCredentials->string, authCredentials->length) == 0)
            return 0;
        code = buildClientAuthHeaders(url, "incorrect", &message, &headers);
    } else {
        code = buildClientAuthHeaders(url, NULL, NULL, &headers);
        message = internAtom("Unexpected authentication scheme");
    }

    *message_return = message;
    *headers_return = headers;
    return code;
}

int
buildServerAuthHeaders(char* buf, int n, int size, AtomPtr authCredentials)
{
    char authbuf[4 * 128 + 3];
    int authlen;

    if(authCredentials->length >= 3 * 128)
        return -1;
    authlen = b64cpy(authbuf, parentAuthCredentials->string,
                     parentAuthCredentials->length, 0);
    n = snnprintf(buf, n, size, "\r\nProxy-Authorization: Basic ");
    n = snnprint_n(buf, n, size, authbuf, authlen);
    return n;
}
