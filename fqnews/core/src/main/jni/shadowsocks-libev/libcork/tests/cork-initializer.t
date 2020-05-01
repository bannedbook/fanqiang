We need to sort the output, since there's no guarantee about which order our
initializer functions will run in.

  $ cork-initializer | sort
  Initializer 1
  Initializer 2
