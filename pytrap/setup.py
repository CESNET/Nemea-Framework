from distutils.core import setup, Extension

pytrapmodule = Extension('pytrap',
                    sources = ['pytrapmodule.c', 'unirecmodule.c', 'fields.c'],
                    libraries = ['trap', 'unirec'])

setup(name = 'nemea-pytrap',
       version = '0.9.2',
       description = 'The pytrap module is a native Python extension that allows for writing NEMEA modules in Python.',
       author = 'Tomas Cejka',
       author_email = 'cejkat@cesnet.cz',
       url = 'https://github.com/CESNET/Nemea-Framework',
       license = 'BSD',
       ext_modules = [pytrapmodule])

