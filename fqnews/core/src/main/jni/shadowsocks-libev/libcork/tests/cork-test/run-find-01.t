  $ cork-test mkdir --recursive a/b/c/b

  $ cork-test find b a
  a/b
  $ cork-test find b a/b/c
  a/b/c/b
  $ cork-test find b a:a/b/c
  a/b
  $ cork-test find b a/b/c:a
  a/b/c/b

  $ cork-test find b/c a
  a/b/c
  $ cork-test find b/c a/b/c
  b/c not found in a/b/c
  [1]

  $ cork-test find d a
  d not found in a
  [1]
  $ cork-test find d a:a/b:a/b/c
  d not found in a:a/b:a/b/c
  [1]
