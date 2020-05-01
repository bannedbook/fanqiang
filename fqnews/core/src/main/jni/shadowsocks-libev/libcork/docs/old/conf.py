# -*- coding: utf-8 -*-

import sys, os

extensions = ['sphinx.ext.mathjax']
source_suffix = '.rst'
master_doc = 'index'
project_name = u'libcork'
project_slug = u'libcork'
company = u'RedJack, LLC'
copyright_years = u'2011-2012'

default_role = 'c:func'
primary_domain = 'c'

rst_epilog = """
.. |project_name| replace:: """ + project_name + """
"""

# Intersphinx stuff

# If your documentation uses intersphinx to link to other Sphinx
# documentation sets, uncomment and fill in the following.
#
#intersphinx_mapping = {
#    'libcork': ('http://libcork.readthedocs.org/en/latest/', None),
#}

# Our CMake build scripts will insert overrides below if the prereq
# libraries have installed their Sphinx documentation locally.  DO NOT
# uncomment out the last line of this block; we need it commented so
# that this conf.py file still works if CMake doesn't do its
# substitution thing.
# @INTERSPHINX_OVERRIDES@

#----------------------------------------------------------------------
# Everything below here shouldn't need to be changed.

release = None
version = None

# Give CMake a chance to insert a version number
# @VERSION_FOR_CONF_PY@

# Otherwise grab version from git
if version is None:
    import re
    import subprocess
    release = str(subprocess.check_output(["git", "describe"]).rstrip())
    version = re.sub(r"-dev.*$", "-dev", release)

# Project details

project = project_name
copyright = copyright_years+u', '+company
templates_path = ['_templates']
exclude_patterns = ['_build']
pygments_style = 'sphinx'

html_theme = 'default'
html_style = 'docco-sphinx.css'
html_static_path = ['_static']
htmlhelp_basename = project_slug+'-doc'


latex_documents = [
  ('index', project_slug+'.tex', project_name+u' Documentation',
   company, 'manual'),
]

man_pages = [
    ('index', 'libcork', u'libcork documentation',
     [u'RedJack, LLC'], 1)
]

texinfo_documents = [
  ('index', 'libcork', u'libcork documentation',
   u'RedJack, LLC', 'libcork', 'One line description of project.',
   'Miscellaneous'),
]
