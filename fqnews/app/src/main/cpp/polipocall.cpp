#include "org_bannedbook_app_service_NativeCall.h"

extern "C" int polipo_main(int argc, const char *argv[]);

extern "C" void Java_org_bannedbook_app_service_NativeCall_execPolipo(JNIEnv * env, jclass,jstring jconfpath)
{
    const char *cconfpath = env->GetStringUTFChars(jconfpath , NULL ) ;
    const char *argv[] = {
            "org_bannedbook_app_service_NativeCall",
            "-c", cconfpath,
    };
    polipo_main(sizeof(argv) / sizeof(argv[0]), argv);
}