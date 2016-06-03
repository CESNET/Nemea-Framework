from distutils.core import setup

DESCRIPTION="Common Python modules and methods of the NEMEA system."
LONG_DESCRIPTION="""The module contains methods for creation and submission of incident reports in IDEA format."""
setup(name='nemea-pycommon',
      version='1.0.5',
      py_modules=['report2idea'],
      packages=[],
      author='Vaclav Bartos, CESNET',
      author_email='bartos@cesnet.cz',
      license="BSD",
      description=DESCRIPTION,
      long_description=LONG_DESCRIPTION
      )

