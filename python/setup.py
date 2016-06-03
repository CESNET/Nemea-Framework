from distutils.core import setup

DESCRIPTION = "Python wrapper for NEMEA Framework"
LONG_DESCRIPTION = """This distribution contains two related modules/packages:
  trap: Python wrapper for libtrap - an implementation of Traffic Analysis
        Platform (TRAP)
  unirec: Python version of a data structure used in TRAP
"""
setup(name='nemea-python',
      version='2.1.0',
      py_modules=['trap'],
      packages=['unirec'],
      author='Vaclav Bartos, CESNET',
      author_email='bartos@cesnet.cz',
      license="BSD",
      description=DESCRIPTION,
      long_description=LONG_DESCRIPTION
      )
