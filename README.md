# OpenSees Source Code Repository [![CMake Build](https://github.com/OpenSees/OpenSees/actions/workflows/build_cmake.yml/badge.svg)](https://github.com/OpenSees/OpenSees/actions/workflows/build_cmake.yml)

This git repository contains all revisions to OpenSees source code since Version 2.3.2.

Older revisions of the code are available upon request.

If you plan on collaborating or even using OpenSees as your base code it is highly recommended that
you FORK this repo to your own account and work on it there. We will not allow anybody to write to
this repo. Only PULL requests will be considered. To fork the repo click on the FORK at the top of this page.

For a brief outline on forking we suggest:
https://www.atlassian.com/git/tutorials/comparing-workflows/forking-workflow

For a brief introduction to using your new FORK we suggest:
https://www.atlassian.com/git/tutorials/saving-changes

## Documentation
The documentation for OpenSees is being moved to a parellel github repo:
https://github.com/OpenSees/OpenSeesDocumentation

The documentation (in its present form) can be viewed in the browser using the following url:
https://OpenSees.github.io/OpenSeesDocumentation

## Build Instructions
Steps to build OpenSees on Windows, Linux, and Mac:
https://opensees.github.io/OpenSeesDocumentation/developer/build.html

### Windows: build for multiple Python versions

Edit the Python list at the top of `build.bat`, then run it from the repository root. Each row has
the form `version|python.exe|Include directory|pythonXY.lib`. To add another version, add one
`set "OPENSEES_PYTHON_315=3.15|D:\Python315\python.exe|D:\Python315\Include|D:\Python315\libs\python315.lib"`
row with the paths for that installation. The script configures a separate
`build/pyXY/Release` CMake tree for each version and places version-tagged extension modules in
`build/python-modules`. It also builds `OpenSees.exe` in the first Python build tree. Existing
build trees are retained for incremental builds.

Use `build.bat -ListPython` to verify all configured paths. Run `build.bat -ConfigureOnly` to
configure the CMake trees without compiling. Each Python installation must include `Python.h`
and its matching Release import library (`libs/pythonXY.lib`).

For example, test the shared module directory with
`set PYTHONPATH=%CD%\build\python-modules` followed by `py -3.11 -c "import opensees; print(opensees.__file__)"`.

## Modeling Questions
Issues related to modeling questions will be closed. Instead, post your modeling questions on the OpenSees 
message board or in the OpenSees Facebook group.
+ https://opensees.berkeley.edu/community
+ https://facebook.com/groups/opensees
