# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2010, RedJack, LLC.
# All rights reserved.
#
# Please see the LICENSE.txt file in this distribution for license
# details.
# ----------------------------------------------------------------------

__all__ = (
    "PACKAGE_DIR",
    "test_file",
)

import os.path

THIS_DIR = os.path.dirname(__file__)
PACKAGE_DIR_REL = os.path.join(THIS_DIR, "..", "..", "..")
PACKAGE_DIR = os.path.abspath(PACKAGE_DIR_REL)
"""The location of the root directory of the source package"""

def test_file(rel_path):
    """
    Return the absolute path of the given test file.
    """

    return os.path.join(PACKAGE_DIR, "tests", rel_path)
