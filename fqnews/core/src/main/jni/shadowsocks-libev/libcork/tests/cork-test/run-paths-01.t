  $ HOME= \
  >     cork-test paths
  Cannot determine home directory
  [1]

  $ HOME=/home/test \
  > XDG_CONFIG_HOME= \
  > XDG_DATA_HOME= \
  > XDG_CONFIG_DIRS= \
  > XDG_DATA_DIRS= \
  > XDG_CACHE_HOME= \
  > XDG_RUNTIME_DIR= \
  >     cork-test paths
  Cannot determine user-specific runtime directory
  Home:    /home/test
  Config:  /home/test/.config:/etc/xdg
  Data:    /home/test/.local/share:/usr/local/share:/usr/share
  Cache:   /home/test/.cache
  [1]

  $ HOME=/home/test \
  > XDG_CONFIG_HOME= \
  > XDG_DATA_HOME= \
  > XDG_CONFIG_DIRS= \
  > XDG_DATA_DIRS= \
  > XDG_CACHE_HOME= \
  > XDG_RUNTIME_DIR=/run/user/test \
  >     cork-test paths
  Home:    /home/test
  Config:  /home/test/.config:/etc/xdg
  Data:    /home/test/.local/share:/usr/local/share:/usr/share
  Cache:   /home/test/.cache
  Runtime: /run/user/test

  $ HOME=/home/test \
  > XDG_CONFIG_HOME=/home/test/custom-config \
  > XDG_DATA_HOME=/home/test/share \
  > XDG_CONFIG_DIRS=/etc \
  > XDG_DATA_DIRS=/usr/share \
  > XDG_CACHE_HOME=/tmp/cache/test \
  > XDG_RUNTIME_DIR=/run/user/test \
  >     cork-test paths
  Home:    /home/test
  Config:  /home/test/custom-config:/etc
  Data:    /home/test/share:/usr/share
  Cache:   /tmp/cache/test
  Runtime: /run/user/test
