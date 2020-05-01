  $ cork-test mkdir --recursive --require a/b/c
  $ cork-test mkdir --recursive --require a/b/d
  $ find a 2>/dev/null | sort
  a
  a/b
  a/b/c
  a/b/d

  $ cork-test rm --require a/b/d
  $ find a 2>/dev/null | sort
  a
  a/b
  a/b/c

  $ cork-test rm --require a/b/d
  No such file or directory
  [1]
  $ find a 2>/dev/null | sort
  a
  a/b
  a/b/c

  $ cork-test rm --require --recursive a/b/c
  $ find a 2>/dev/null | sort
  a
  a/b

  $ cork-test rm --recursive a
  $ find a 2>/dev/null | sort
