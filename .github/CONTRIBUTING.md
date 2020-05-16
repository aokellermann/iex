### Code Style
C++ code in this repository follows [Google C++ Style](https://google.github.io/styleguide/cppguide.html) fairly closely. Divergences and further specifications are laid out below:
* Maximum line length is 120 characters.
* Use `#pragma once` instead of header guards.
* Braces go on a new line, unless the entire expression can fit on one line.
* File names never use dashes.
* Doxygen is used for documentation.
* Exceptions may be thrown.
* Full header path in includes doesn't need to be specified.
When in doubt, follow conventions in preexisting code.

You code's conformity to this repo's style can be checked with `style.sh`. Any nonconformities will be printed to output. Dependencies for this script are:
* [cpplint](https://raw.githubusercontent.com/google/styleguide/gh-pages/cpplint/cpplint.py)
* clang-format 10.0.0