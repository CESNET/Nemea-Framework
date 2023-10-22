from setuptools import setup, Extension
import os

SRC_PATH = os.path.relpath(os.path.join(os.path.dirname(__file__), ".", "src"))

name = 'nemea-pytrap'
version = '0.16.0'
release = version
description = 'Python extension of the NEMEA project.'
long_description = 'The pytrap module is a native Python extension that allows for writing NEMEA modules in Python.'
author = 'Tomas Cejka'
author_email = 'cejkat@cesnet.cz'
maintainer = 'Tomas Cejka'
maintainer_email = 'cejkat@cesnet.cz'

pytrapmodule = Extension('pytrap.pytrap',
                    sources = ['src/pytrapmodule.c', 'src/unirecmodule.c', 'src/unirecipaddr.c', 'src/unirecmacaddr.c', 'src/fields.c'],
                    libraries = ['trap', 'unirec'])

try:
    from sphinx.setup_command import BuildDoc
    cmdclass = {'build_sphinx': BuildDoc}
    command_options = {'build_sphinx': {
               'project': ('setup.py', name),
               'version': ('setup.py', version),
               'release': ('setup.py', release),
               'source_dir': ('setup.py', 'docs'),
               'build_dir': ('setup.py', 'dist/doc/')}
               }
except:
    cmdclass = {}
    command_options = {}

setup(name = name,
       version = version,
       description = description,
       long_description = long_description,
       author = author,
       author_email = author_email,
       maintainer = maintainer,
       maintainer_email = maintainer_email,
       url = 'https://github.com/CESNET/Nemea-Framework',
       license = 'BSD',
       test_suite = "test",
       platforms = ["Linux"],
       classifiers = [
              'Development Status :: 4 - Beta',
              'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
              'Operating System :: POSIX :: Linux',
              'Programming Language :: C',
              'Programming Language :: Python :: 3',
              'Programming Language :: Python :: 3.6',
              'Programming Language :: Python :: Implementation :: CPython',
              'Topic :: Software Development :: Libraries',
              'Topic :: System :: Networking :: Monitoring'
       ],
       ext_modules = [pytrapmodule],
       packages = ["pytrap"],
       package_dir={ "": SRC_PATH, },
       # sphinx:
       cmdclass=cmdclass,
       # these are optional and override conf.py settings
       command_options=command_options,
       )

