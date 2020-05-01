README for git hooks script
===========================
git has a way to run scripts, which are invoked by specific git commands.
The git hooks are located in `<mbed TLS root>/.git/hooks`, and as such are not under version control
for more information, see the [git documentation](https://git-scm.com/docs/githooks).

The mbed TLS git hooks are located in `<mbed TLS root>/tests/git-scripts` directory, and one must create a soft link from `<mbed TLS root>/.git/hooks` to `<mbed TLS root>/tesst/git-scripts`, in order to make the hook scripts successfully work.

Example:

Execute the following command to create a link on linux from the mbed TLS `.git/hooks` directory:  
`ln -s ../../tests/git-scripts/pre-push.sh pre-push`

**Note: Currently the mbed TLS git hooks work only on a GNU platform. If using a non-GNU platform, don't enable these hooks!**

These scripts can also be used independently.
