This first test makes sure to only create one file or subdirectory in each
directory.  That means that we don't have to sort the output to get a
reproducible result, which lets us check the ordering of the different events in
the callback stucture.

  $ mkdir test1
  $ mkdir test1/a
  $ mkdir test1/a/b
  $ touch test1/a/b/c
  $ cork-test dir test1
  Entering a (a)
    Entering b (a/b)
      c (a/b/c) (test1/a/b/c)
    Leaving a/b
  Leaving a
  $ cork-test dir --shallow test1
  Skipping a

  $ mkdir test2
  $ touch test2/a
  $ cork-test dir test2
  a (a) (test2/a)
  $ cork-test dir --shallow test2
  a (a) (test2/a)

A more complex directory structure.  We have to sort the output, since there's
no guarantee in what order the directory walker will encounter the files.

  $ mkdir test3
  $ mkdir test3/d1
  $ mkdir test3/d2
  $ touch test3/d2/a
  $ touch test3/d2/b
  $ mkdir test3/d3
  $ touch test3/d3/a
  $ touch test3/d3/b
  $ touch test3/d3/c
  $ mkdir test3/d3/s1
  $ mkdir test3/d3/s1/s2
  $ touch test3/d3/s1/s2/a
  $ cork-test dir --only-files test3 | sort
  d2/a
  d2/b
  d3/a
  d3/b
  d3/c
  d3/s1/s2/a

Test what happens when the directory doesn't exit.

  $ cork-test dir missing
  No such file or directory
  [1]
  $ cork-test dir --only-files missing
  No such file or directory
  [1]
  $ cork-test dir --shallow missing
  No such file or directory
  [1]
