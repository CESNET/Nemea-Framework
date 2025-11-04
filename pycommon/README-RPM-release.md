# How to release a new version of the RPM?
1. Bump the version in `pyproject.toml` and `*.spec`.

2. Don't forget to commit and push with a commit message:

   ```
   pycommon: increased version, released package
   ```

3. Create a Python source distribution:
   ```bash
   python3 -m pip install build
   python3 -m build --sdist
   ```

4. Upload files in `dist/` using `twine`:

   ```
   twine upload ./*
   ```

5. Run `make rpm`
6. Your packages are in `RPMBUILD/`, build for other RPM-based systems can be done using:

   ```
   copr build @CESNET/NEMEA RPMBUILD/SRPMS/<package.src.rpm>
   ```

