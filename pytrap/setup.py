from setuptools import setup, Extension
import os

SRC_PATH = os.path.relpath(os.path.join(os.path.dirname(__file__), "."))

pytrapmodule = Extension('pytrap',
                    sources = ['src/pytrapmodule.c', 'src/unirecmodule.c', 'src/unirecipaddr.c', 'src/fields.c'],
                    libraries = ['trap', 'unirec'])

setup(name = 'nemea-pytrap',
       version = '0.9.12',
       description = 'Python extension of the NEMEA project.',
       long_description = 'The pytrap module is a native Python extension that allows for writing NEMEA modules in Python.',
       author = 'Tomas Cejka',
       author_email = 'cejkat@cesnet.cz',
       maintainer = 'Tomas Cejka',
       maintainer_email = 'cejkat@cesnet.cz',
       url = 'https://github.com/CESNET/Nemea-Framework',
       license = 'BSD',
       test_suite = "test",
       platforms = ["Linux"],
       classifiers = [
              'Development Status :: 4 - Beta',
              'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
              'Operating System :: POSIX :: Linux',
              'Programming Language :: C',
              'Programming Language :: Python :: 2',
              'Programming Language :: Python :: 2.7',
              'Programming Language :: Python :: 3',
              'Programming Language :: Python :: 3.4',
              'Programming Language :: Python :: Implementation :: CPython',
              'Topic :: Software Development :: Libraries',
              'Topic :: System :: Networking :: Monitoring'
       ],
       ext_modules = [pytrapmodule],
       package_dir={ "": SRC_PATH, },
       )

