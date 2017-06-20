from setuptools import setup

DESCRIPTION="Common Python modules and methods of the NEMEA system."
LONG_DESCRIPTION="""The module contains methods for creation and submission of incident reports in IDEA format."""
setup(name='nemea-pycommon',
      version='1.1.0',
      py_modules=['report2idea', 'ip_prefix_search'],
      packages=['reporter_config'],
      author='Vaclav Bartos, CESNET',
      author_email='bartos@cesnet.cz',
      maintainer = 'Tomas Cejka',
      maintainer_email = 'cejkat@cesnet.cz',
      url = 'https://github.com/CESNET/Nemea-Framework',
      license="BSD",
      test_suite="test",
      platforms = ["any"],
      install_requires = [ 'pynspect', 'idea-format', 'PyYAML', 'netaddr' ],
      classifiers = [
              'Development Status :: 4 - Beta',
              'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
              'Operating System :: POSIX',
              'Programming Language :: C',
              'Programming Language :: Python',
              'Programming Language :: Python :: 2',
              'Programming Language :: Python :: 3',
              'Topic :: Software Development :: Libraries',
              'Topic :: System :: Networking :: Monitoring'
      ],
      description=DESCRIPTION,
      long_description=LONG_DESCRIPTION
)

