# How to release new version of RPM?

1. bump version in `setup.py` and `*.spec`
2. don't forget to commit and push with commit message

   ```
   pycommon: increased version, released package
   ```

3. create python src package using `python setup.py sdist`
4. sign package(s) in `dist/` using:

   ```
   for i in *; do gpg2 --detach-sign -a "$i"; done
   ```

5. upload files in `dist/` using `twine`:

   ```
   twine upload ./*
   ```

6. run `make rpm`
7. your packages are in `RPMBUILD/`, build for other RPM-based systems can be done using:

   ```
   copr build @CESNET/NEMEA RPMBUILD/SRPMS/<package.src.rpm>
   ```

