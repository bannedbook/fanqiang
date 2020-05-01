  $ cork-test mkdir a
  $ find a | sort
  a

  $ cork-test mkdir a
  $ find a | sort
  a

  $ cork-test mkdir --require a
  File exists
  [1]
  $ find a | sort
  a

  $ cork-test mkdir --recursive a/b/c
  $ find a | sort
  a
  a/b
  a/b/c

  $ cork-test mkdir --recursive a/b
  $ find a | sort
  a
  a/b
  a/b/c

  $ cork-test mkdir --recursive --require a/b
  File exists
  [1]
  $ find a | sort
  a
  a/b
  a/b/c

  $ cork-test mkdir --recursive --require a/b/d
  $ find a | sort
  a
  a/b
  a/b/c
  a/b/d
