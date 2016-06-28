from distutils.core import setup, Extension

pytrapmodule = Extension('pytrap',
                    sources = ['pytrapmodule.c', 'unirecmodule.c', 'fields.c'],
                    libraries = ['trap', 'unirec'])

setup(name = 'pytrap',
       version = '1.0',
       description = 'The pytrap module is a native Python extension that allows for writing NEMEA modules in Python.',
       author = 'Tomas Cejka',
       author_email = 'cejkat@cesnet.cz',
       url = 'https://github.com/CESNET/Nemea-Framework',
       ext_modules = [pytrapmodule])

