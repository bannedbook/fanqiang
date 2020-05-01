#ifdef __MINGW32__

#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "libcork/core.h"
#include "libcork/ds.h"
#include "libcork/os/subprocess.h"
#include "libcork/helpers/mingw.h"

// From git project: compact/mingw.c

struct tm
*gmtime_r(const time_t *timep, struct tm *result)
{
    /* gmtime() in MSVCRT.DLL is thread-safe, but not reentrant */
    memcpy(result, gmtime(timep), sizeof(struct tm));
    return result;
}

struct tm
*localtime_r(const time_t *timep, struct tm *result)
{
    /* localtime() in MSVCRT.DLL is thread-safe, but not reentrant */
    memcpy(result, localtime(timep), sizeof(struct tm));
    return result;
}

int
setenv(const char *name, const char *value, int overwrite)
{
    // XXX: ignore overwrite option which is not actually used by MinGW port
    errno_t ret;
    ret = _putenv_s(name, value);
    if (ret) {
        return -1;
    }
    return 0;
}

int
unsetenv(const char *name)
{
    return setenv(name, "", 1);
}

int
clearenv(void)
{
    // XXX: stub function, not actually used by MinGW port
    return 0;
}

// Missing subprocess related functions

/*-----------------------------------------------------------------------
 * Subprocesses
 */

#define BUF_SIZE  4096

struct cork_subprocess {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    HANDLE job;
    uint16_t control;
    bool running;
    void  *user_data;
    cork_free_f  free_user_data;
    int  *exit_code;
    char  buf[BUF_SIZE];
};

struct cork_subprocess *
cork_subprocess_new(void *user_data, cork_free_f free_user_data,
                    cork_run_f run,
                    struct cork_stream_consumer *stdout_consumer,
                    struct cork_stream_consumer *stderr_consumer,
                    int *exit_code)
{
    struct cork_subprocess  *self = cork_new(struct cork_subprocess);
    memset(&self->si, 0, sizeof(self->si));
    memset(&self->pi, 0, sizeof(self->pi));
    self->control = 0;
    self->running = false;
    self->user_data = user_data;
    self->free_user_data = free_user_data;
    self->exit_code = exit_code;
    return self;
}

void
cork_subprocess_free(struct cork_subprocess *self)
{
    cork_free_user_data(self);
    cork_delete(struct cork_subprocess, self);
}

/*-----------------------------------------------------------------------
 * Executing another program
 */

static void
cork_exec__free(void *vself)
{
    struct cork_exec  *exec = vself;
    cork_exec_free(exec);
}

struct cork_subprocess *
cork_subprocess_new_exec(struct cork_exec *exec,
                         struct cork_stream_consumer *out,
                         struct cork_stream_consumer *err,
                         int *exit_code)
{
    // XXX: Consumer out and err are not used in MinGW port
    return cork_subprocess_new
        (exec, cork_exec__free,
         NULL,
         NULL, NULL, exit_code);
}

void
cork_subprocess_set_control(struct cork_subprocess *self, uint16_t port)
{
    self->control = port;
}

/*-----------------------------------------------------------------------
 * Running subprocesses
 */

DWORD WINAPI
cork_subprocess_monitor_thread(LPVOID data)
{
    struct cork_subprocess *self = data;
    SOCKET fd;
    struct sockaddr_in addr;
    char buf[1] = {0};

    if (self->running) {
        // Wait subprocess to exit
        WaitForSingleObject(self->pi.hProcess, INFINITE);

        // Get exit code
        self->running = false;
        DWORD exitCode;
        if (GetExitCodeProcess(self->pi.hProcess, &exitCode)) {
            if (exitCode != STILL_ACTIVE && self->exit_code) {
                *self->exit_code = exitCode;
            }
        }

        // Clean up handles
        CloseHandle(self->pi.hProcess);
        CloseHandle(self->pi.hThread);
        if (self->job) {
            CloseHandle(self->job);
        }
    }

    // Notify control port
    if (self->control == 0) {
        return 0;
    }
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == INVALID_SOCKET) {
        return 1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(self->control);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr))) {
        closesocket(fd);
        return 2;
    };
    send(fd, buf, 1, 0);
    closesocket(fd);
    return 0;
}

int
cork_subprocess_start(struct cork_subprocess *self)
{
    int ret = 0;
    struct cork_exec *exec = (struct cork_exec *)self->user_data;
    const char *exec_desc = cork_exec_description(exec);
    struct cork_buffer command;
    char *command_buf = NULL;

    memset(&self->si, 0, sizeof(self->si));
    self->si.cb = sizeof(self->si);
    memset(&self->pi, 0, sizeof(self->pi));
    cork_buffer_init(&command);

    /* Copy command line */
    if (exec_desc != NULL) {
        cork_buffer_set_string(&command, exec_desc);
    }
    command_buf = (char *)command.buf;

    /* Detect if current process already in a job */
    BOOL isInJob = FALSE;
    DWORD creationFlags = 0;
    if (IsProcessInJob(GetCurrentProcess(), NULL, &isInJob)) {
        if (isInJob) {
            creationFlags |= CREATE_BREAKAWAY_FROM_JOB;
        }
    };

    /* Stop subprocess when current process exits */
    self->job = CreateJobObject(NULL, NULL);
    BOOL retJob = FALSE;
    if (self->job) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = {0};
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_BREAKAWAY_OK;
        retJob = SetInformationJobObject(self->job, JobObjectExtendedLimitInformation, &info, sizeof(info));
    }

    /* Execute the new program */
    self->running = false;
    if (!CreateProcess(
            NULL,           // No module name (use command line)
            command_buf,    // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            creationFlags,  // Creation flags
            NULL,           // Use current environment block
            NULL,           // Use current starting directory
            &self->si,      // Pointer to STARTUPINFO structure
            &self->pi)) {   // Pointer to PROCESS_INFORMATION structure
        ret = -1;
    } else {
        self->running = true;
        if (self->job && retJob) {
            AssignProcessToJobObject(self->job, self->pi.hProcess);
        }
    }

    /* Monitor if subprocess quits in a separate thread */
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)cork_subprocess_monitor_thread,
                 self, 0, NULL);

    cork_buffer_done(&command);
    return ret;
}

int
cork_subprocess_abort(struct cork_subprocess *self)
{
    int ret = 0;
    if (self->running) {
        if (!TerminateProcess(self->pi.hProcess, 0)) {
            ret = -1;
        }
    }
    return ret;
}

bool
cork_subprocess_is_finished(struct cork_subprocess *self)
{
    return self->running;
}

#endif
