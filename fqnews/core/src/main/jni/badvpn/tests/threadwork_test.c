#include <stddef.h>
#include <stdio.h>

#include <misc/debug.h>
#include <base/BLog.h>
#include <base/DebugObject.h>
#include <threadwork/BThreadWork.h>

BReactor reactor;
BThreadWorkDispatcher twd;
BThreadWork tw1;
BThreadWork tw2;
BThreadWork tw3;
int num_left;

static void handler_done (void *user)
{
    printf("work done\n");
    
    num_left--;
    
    if (num_left == 0) {
        printf("all works done, quitting\n");
        BReactor_Quit(&reactor, 0);
    }
}

static void work_func (void *user)
{
    unsigned int x = 0;
    
    for (int i = 0; i < 10000; i++) {
        for (int j = 0; j < 10000; j++) {
            x++;
        }
    }
}

static void dummy_works (int n)
{
    for (int i = 0; i < n; i++) {
        BThreadWork tw_tmp;
        BThreadWork_Init(&tw_tmp, &twd, handler_done, NULL, work_func, NULL);
        BThreadWork_Free(&tw_tmp);
    }
}

int main ()
{
    BLog_InitStdout();
    BLog_SetChannelLoglevel(BLOG_CHANNEL_BThreadWork, BLOG_DEBUG);
    
    if (!BReactor_Init(&reactor)) {
        DEBUG("BReactor_Init failed");
        goto fail1;
    }
    
    if (!BThreadWorkDispatcher_Init(&twd, &reactor, 1)) {
        DEBUG("BThreadWorkDispatcher_Init failed");
        goto fail2;
    }
    
    dummy_works(200);
    
    BThreadWork_Init(&tw1, &twd, handler_done, NULL, work_func, NULL);
    
    BThreadWork_Init(&tw2, &twd, handler_done, NULL, work_func, NULL);
    
    BThreadWork_Init(&tw3, &twd, handler_done, NULL, work_func, NULL);
    
    dummy_works(200);
    
    num_left = 3;
    
    BReactor_Exec(&reactor);
    
    BThreadWork_Free(&tw3);
    BThreadWork_Free(&tw2);
    BThreadWork_Free(&tw1);
    BThreadWorkDispatcher_Free(&twd);
fail2:
    BReactor_Free(&reactor);
fail1:
    BLog_Free();
    DebugObjectGlobal_Finish();
    return 0;
}
