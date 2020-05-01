# Contributing to the libevent

## Coding style

First and most generic rule: **just look around**.

But, we have a script for checking patches/files/git-refs:
```shell
# Chech HEAD git ref
./checkpatch.sh -r
./checkpatch.sh -r HEAD

# Check patch
git format-patch --stdout -1 | ./checkpatch.sh -p
git show -1 | ./checkpatch.sh -p

# Or via regular files
git format-patch --stdout -2
./checkpatch.sh *.patch

# Over a file
./checkpatch.sh -d event.c
./checkpatch.sh -d < event.c

# And print the whole file not only summary
./checkpatch.sh -f event.c
./checkpatch.sh -f < event.c

# See
./checkpatch.sh -h
```

## Testing
- Write new unit test in `test/regress_{MORE_SUITABLE_FOR_YOU}.c`
- `make verify`
