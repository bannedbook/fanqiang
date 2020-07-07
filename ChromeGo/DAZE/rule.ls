# RULE file aims to be a minimal configuration file format that's easy to
# read due to obvious semantics.
# There are two parts per line on RULE file: mode and glob. mode are on the
# left of the space sign and glob are on the right. mode is an char and
# describes whether the host should go proxy, glob supported glob-style
# patterns:
#   h?llo matches hello, hallo and hxllo
#   h*llo matches hllo and heeeello
#   h[ae]llo matches hello and hallo, but not hillo
#   h[^e]llo matches hallo, hbllo, ... but not hello
#   h[a-b]llo matches hallo and hbllo
#
# This is a RULE document:
#   L a.com
#   R b.com
#   B c.com
#
# L(ocale)  means using locale network
# R(emote)  means using remote network
# B(anned)  means block it

B hm.baidu.com
