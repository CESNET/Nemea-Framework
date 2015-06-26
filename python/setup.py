from distutils.core import setup

setup(name='trap+unirec',
      version='1.0',
      py_modules=['trap'],
      packages=['unirec'],
      package_data={'unirec':['fields.py']},
      author='Vaclav Bartos, CESNET',
      author_email='bartos@cesnet.cz',
      license="BSD",
      description="""This distribution contains two related modules/packages:
  trap: Python wrapper for libtrap - an implementation of Traffic Analysis Platform (TRAP)
  unirec: Python version of a data structure used in TRAP
""",
      )

