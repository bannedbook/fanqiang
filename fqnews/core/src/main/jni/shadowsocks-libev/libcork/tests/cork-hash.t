Make sure that our stable hash is really stable.

  $ cork-hash foo
  0xf6a5c420
  $ cork-hash bar
  0x450e998d
  $ cork-hash tests.h
  0x0b0628ee
  $ cork-hash "A longer string"
  0x53a2c885
