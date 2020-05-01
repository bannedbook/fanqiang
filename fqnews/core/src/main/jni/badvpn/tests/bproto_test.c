#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include <misc/debug.h>
#include <misc/balloc.h>

#include <generated/bproto_bproto_test.h>

int main ()
{
    uint16_t a = 17501;
    uint64_t c = 82688926;
    uint16_t d1 = 1517;
    uint16_t d2 = 1518;
    uint8_t e = 72;
    const char *f = "hello world";
    const char *g = "helo";
    
    // encode message
    
    int len = msg1_SIZEa + msg1_SIZEc + msg1_SIZEd + msg1_SIZEd + msg1_SIZEe + msg1_SIZEf(strlen(f)) + msg1_SIZEg;
    
    uint8_t *msg = (uint8_t *)BAlloc(len);
    ASSERT_FORCE(msg)
    msg1Writer writer;
    msg1Writer_Init(&writer, msg);
    msg1Writer_Adda(&writer, a);
    msg1Writer_Addc(&writer, c);
    msg1Writer_Addd(&writer, d1);
    msg1Writer_Addd(&writer, d2);
    msg1Writer_Adde(&writer, e);
    uint8_t *f_dst = msg1Writer_Addf(&writer, strlen(f));
    memcpy(f_dst, f, strlen(f));
    uint8_t *g_dst = msg1Writer_Addg(&writer);
    memcpy(g_dst, g, strlen(g));
    int len2 = msg1Writer_Finish(&writer);
    ASSERT_EXECUTE(len2 == len)
    
    // parse message
    
    msg1Parser parser;
    ASSERT_EXECUTE(msg1Parser_Init(&parser, msg, len))
    
    // check parse results
    
    uint16_t p_a;
    uint64_t p_c;
    uint16_t p_d1;
    uint16_t p_d2;
    uint8_t p_e;
    uint8_t *p_f;
    int p_f_len;
    uint8_t *p_g;
    ASSERT_EXECUTE(msg1Parser_Geta(&parser, &p_a))
    ASSERT_EXECUTE(msg1Parser_Getc(&parser, &p_c))
    ASSERT_EXECUTE(msg1Parser_Getd(&parser, &p_d1))
    ASSERT_EXECUTE(msg1Parser_Getd(&parser, &p_d2))
    ASSERT_EXECUTE(msg1Parser_Gete(&parser, &p_e))
    ASSERT_EXECUTE(msg1Parser_Getf(&parser, &p_f, &p_f_len))
    ASSERT_EXECUTE(msg1Parser_Getg(&parser, &p_g))
    
    ASSERT(p_a == a)
    ASSERT(p_c == c)
    ASSERT(p_d1 == d1)
    ASSERT(p_d2 == d2)
    ASSERT(p_e == e)
    ASSERT(p_f_len == strlen(f) && !memcmp(p_f, f, p_f_len))
    ASSERT(!memcmp(p_g, g, strlen(g)))
    
    ASSERT(msg1Parser_GotEverything(&parser))
    
    BFree(msg);
    
    return 0;
}
