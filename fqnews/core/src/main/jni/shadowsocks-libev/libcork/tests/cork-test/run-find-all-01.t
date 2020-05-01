  $ cork-test mkdir --recursive a/b/c/b

  $ cork-test find --all b a
  a/b
  $ cork-test find --all b a/b/c
  a/b/c/b
  $ cork-test find --all b a:a/b/c
  a/b
  a/b/c/b
  $ cork-test find --all b a/b/c:a
  a/b/c/b
  a/b

  $ cork-test find --all b/c a
  a/b/c
  $ cork-test find --all b/c a/b/c

  $ cork-test find --all d a
  $ cork-test find --all d a:a/b:a/b/c
