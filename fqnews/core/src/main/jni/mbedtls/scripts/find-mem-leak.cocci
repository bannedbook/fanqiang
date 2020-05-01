@@
expression x, y;
statement S;
@@
  x = mbedtls_calloc(...);
  y = mbedtls_calloc(...);
  ...
* if (x == NULL || y == NULL)
    S

@@
expression x, y;
statement S;
@@
  if (
*   (x = mbedtls_calloc(...)) == NULL
    ||
*   (y = mbedtls_calloc(...)) == NULL
  )
    S
