/*
Copyright (c) 2003-2006 by Juliusz Chroboczek

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

int httpAccept(int, FdEventHandlerPtr, AcceptRequestPtr);
void httpClientFinish(HTTPConnectionPtr connection, int s);
int httpClientHandler(int, FdEventHandlerPtr, StreamRequestPtr);
int httpClientNoticeError(HTTPRequestPtr, int code, struct _Atom *message);
int httpClientError(HTTPRequestPtr, int code, struct _Atom *message);
int httpClientNewError(HTTPConnectionPtr, int method, int persist, 
                       int code, struct _Atom *message);
int httpClientRawError(HTTPConnectionPtr, int, struct _Atom*, int close);
int httpErrorStreamHandler(int status,
                           FdEventHandlerPtr event,
                           StreamRequestPtr request);
int httpErrorNocloseStreamHandler(int status,
                                  FdEventHandlerPtr event,
                                  StreamRequestPtr request);
int httpErrorNofinishStreamHandler(int status,
                                   FdEventHandlerPtr event,
                                   StreamRequestPtr request);
int httpClientRequest(HTTPRequestPtr request, AtomPtr url);
int httpClientRequestContinue(int forbidden_code, AtomPtr url,
                              AtomPtr forbidden_message,
                              AtomPtr forbidden_headers,
                              void *closure);
int httpClientDiscardBody(HTTPConnectionPtr connection);
int httpClientDiscardHandler(int, FdEventHandlerPtr, StreamRequestPtr);
int httpClientGetHandler(int, ConditionHandlerPtr);
int httpClientHandlerHeaders(FdEventHandlerPtr event, 
                                StreamRequestPtr request,
                                HTTPConnectionPtr connection);
int httpClientNoticeRequest(HTTPRequestPtr request, int);
int httpServeObject(HTTPConnectionPtr);
int delayedHttpServeObject(HTTPConnectionPtr connection);
int httpServeObjectStreamHandler(int status, 
                                 FdEventHandlerPtr event,
                                 StreamRequestPtr request);
int httpServeObjectStreamHandler2(int status, 
                                  FdEventHandlerPtr event,
                                  StreamRequestPtr request);
int httpServeObjectHandler(int, ConditionHandlerPtr);
int httpClientSideRequest(HTTPRequestPtr request);
int  httpClientSideHandler(int status,
                           FdEventHandlerPtr event,
                           StreamRequestPtr srequest);
