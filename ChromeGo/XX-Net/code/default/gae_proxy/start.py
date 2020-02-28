#!/usr/bin/env python
# coding:utf-8

import os
import sys

__file__ = os.path.abspath(__file__)
if os.path.islink(__file__):
    __file__ = getattr(os, 'readlink', lambda x: x)(__file__)

current_path = os.path.dirname(os.path.abspath(__file__))
sys.path.append(current_path)
import local.proxy as client


def main(args):
    try:
        client.main(args)
    except KeyboardInterrupt:
        sys.exit()

if __name__ == "__main__":
    main({})
