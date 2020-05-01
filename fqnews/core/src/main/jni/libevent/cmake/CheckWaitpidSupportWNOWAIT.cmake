include(CheckCSourceRuns)

check_c_source_runs(
"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

int
main(int argc, char** argv)
{
    pid_t pid;
    int status;
    if ((pid = fork()) == 0) _exit(0);
    _exit(waitpid(pid, &status, WNOWAIT) == -1);
}"
EVENT__HAVE_WAITPID_WITH_WNOWAIT)
