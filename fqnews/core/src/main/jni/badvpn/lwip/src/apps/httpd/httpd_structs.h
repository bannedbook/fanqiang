#ifndef LWIP_HTTPD_STRUCTS_H
#define LWIP_HTTPD_STRUCTS_H

#include "lwip/apps/httpd.h"

#if LWIP_HTTPD_DYNAMIC_HEADERS
/** This struct is used for a list of HTTP header strings for various
 * filename extensions. */
typedef struct {
  const char *extension;
  const char *content_type;
} tHTTPHeader;

/** A list of strings used in HTTP headers (see RFC 1945 HTTP/1.0 and
 * RFC 2616 HTTP/1.1 for header field definitions) */
static const char *const g_psHTTPHeaderStrings[] = {
  "HTTP/1.0 200 OK\r\n",
  "HTTP/1.0 404 File not found\r\n",
  "HTTP/1.0 400 Bad Request\r\n",
  "HTTP/1.0 501 Not Implemented\r\n",
  "HTTP/1.1 200 OK\r\n",
  "HTTP/1.1 404 File not found\r\n",
  "HTTP/1.1 400 Bad Request\r\n",
  "HTTP/1.1 501 Not Implemented\r\n",
  "Content-Length: ",
  "Connection: Close\r\n",
  "Connection: keep-alive\r\n",
  "Connection: keep-alive\r\nContent-Length: ",
  "Server: "HTTPD_SERVER_AGENT"\r\n",
  "\r\n<html><body><h2>404: The requested file cannot be found.</h2></body></html>\r\n"
#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
  , "Connection: keep-alive\r\nContent-Length: 77\r\n\r\n<html><body><h2>404: The requested file cannot be found.</h2></body></html>\r\n"
#endif
};

/* Indexes into the g_psHTTPHeaderStrings array */
#define HTTP_HDR_OK             0 /* 200 OK */
#define HTTP_HDR_NOT_FOUND      1 /* 404 File not found */
#define HTTP_HDR_BAD_REQUEST    2 /* 400 Bad request */
#define HTTP_HDR_NOT_IMPL       3 /* 501 Not Implemented */
#define HTTP_HDR_OK_11          4 /* 200 OK */
#define HTTP_HDR_NOT_FOUND_11   5 /* 404 File not found */
#define HTTP_HDR_BAD_REQUEST_11 6 /* 400 Bad request */
#define HTTP_HDR_NOT_IMPL_11    7 /* 501 Not Implemented */
#define HTTP_HDR_CONTENT_LENGTH 8 /* Content-Length: (HTTP 1.0)*/
#define HTTP_HDR_CONN_CLOSE     9 /* Connection: Close (HTTP 1.1) */
#define HTTP_HDR_CONN_KEEPALIVE 10 /* Connection: keep-alive (HTTP 1.1) */
#define HTTP_HDR_KEEPALIVE_LEN  11 /* Connection: keep-alive + Content-Length: (HTTP 1.1)*/
#define HTTP_HDR_SERVER         12 /* Server: HTTPD_SERVER_AGENT */
#define DEFAULT_404_HTML        13 /* default 404 body */
#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
#define DEFAULT_404_HTML_PERSISTENT 14 /* default 404 body, but including Connection: keep-alive */
#endif

#define HTTP_CONTENT_TYPE(contenttype) "Content-Type: "contenttype"\r\n\r\n"
#define HTTP_CONTENT_TYPE_ENCODING(contenttype, encoding) "Content-Type: "contenttype"\r\nContent-Encoding: "encoding"\r\n\r\n"

#define HTTP_HDR_HTML           HTTP_CONTENT_TYPE("text/html")
#define HTTP_HDR_SSI            HTTP_CONTENT_TYPE("text/html\r\nExpires: Fri, 10 Apr 2008 14:00:00 GMT\r\nPragma: no-cache")
#define HTTP_HDR_GIF            HTTP_CONTENT_TYPE("image/gif")
#define HTTP_HDR_PNG            HTTP_CONTENT_TYPE("image/png")
#define HTTP_HDR_JPG            HTTP_CONTENT_TYPE("image/jpeg")
#define HTTP_HDR_BMP            HTTP_CONTENT_TYPE("image/bmp")
#define HTTP_HDR_ICO            HTTP_CONTENT_TYPE("image/x-icon")
#define HTTP_HDR_APP            HTTP_CONTENT_TYPE("application/octet-stream")
#define HTTP_HDR_JS             HTTP_CONTENT_TYPE("application/javascript")
#define HTTP_HDR_RA             HTTP_CONTENT_TYPE("application/javascript")
#define HTTP_HDR_CSS            HTTP_CONTENT_TYPE("text/css")
#define HTTP_HDR_SWF            HTTP_CONTENT_TYPE("application/x-shockwave-flash")
#define HTTP_HDR_XML            HTTP_CONTENT_TYPE("text/xml")
#define HTTP_HDR_PDF            HTTP_CONTENT_TYPE("application/pdf")
#define HTTP_HDR_JSON           HTTP_CONTENT_TYPE("application/json")
#define HTTP_HDR_CSV            HTTP_CONTENT_TYPE("text/csv")
#define HTTP_HDR_TSV            HTTP_CONTENT_TYPE("text/tsv")
#define HTTP_HDR_SVG            HTTP_CONTENT_TYPE("image/svg+xml")
#define HTTP_HDR_SVGZ           HTTP_CONTENT_TYPE_ENCODING("image/svg+xml", "gzip")

#define HTTP_HDR_DEFAULT_TYPE   HTTP_CONTENT_TYPE("text/plain")

/** A list of extension-to-HTTP header strings (see outdated RFC 1700 MEDIA TYPES
 * and http://www.iana.org/assignments/media-types for registered content types
 * and subtypes) */
static const tHTTPHeader g_psHTTPHeaders[] = {
  { "html", HTTP_HDR_HTML},
  { "htm",  HTTP_HDR_HTML},
  { "shtml", HTTP_HDR_SSI},
  { "shtm", HTTP_HDR_SSI},
  { "ssi",  HTTP_HDR_SSI},
  { "gif",  HTTP_HDR_GIF},
  { "png",  HTTP_HDR_PNG},
  { "jpg",  HTTP_HDR_JPG},
  { "bmp",  HTTP_HDR_BMP},
  { "ico",  HTTP_HDR_ICO},
  { "class", HTTP_HDR_APP},
  { "cls",  HTTP_HDR_APP},
  { "js",   HTTP_HDR_JS},
  { "ram",  HTTP_HDR_RA},
  { "css",  HTTP_HDR_CSS},
  { "swf",  HTTP_HDR_SWF},
  { "xml",  HTTP_HDR_XML},
  { "xsl",  HTTP_HDR_XML},
  { "pdf",  HTTP_HDR_PDF},
  { "json", HTTP_HDR_JSON},
#ifdef HTTPD_ADDITIONAL_CONTENT_TYPES
  /* If you need to add content types not listed here:
   * #define HTTPD_ADDITIONAL_CONTENT_TYPES {"ct1", HTTP_CONTENT_TYPE("text/ct1")}, {"exe", HTTP_CONTENT_TYPE("application/exe")}
   */
  , HTTPD_ADDITIONAL_CONTENT_TYPES
#endif
};

#define NUM_HTTP_HEADERS (sizeof(g_psHTTPHeaders) / sizeof(tHTTPHeader))

#endif /* LWIP_HTTPD_DYNAMIC_HEADERS */

#if LWIP_HTTPD_SSI
static const char *const g_pcSSIExtensions[] = {
  ".shtml", ".shtm", ".ssi", ".xml"
};
#define NUM_SHTML_EXTENSIONS (sizeof(g_pcSSIExtensions) / sizeof(const char *))
#endif /* LWIP_HTTPD_SSI */

#endif /* LWIP_HTTPD_STRUCTS_H */
