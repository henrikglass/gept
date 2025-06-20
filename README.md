[![Build](https://github.com/henrikglass/gept/actions/workflows/build.yml/badge.svg)](https://github.com/henrikglass/gept/actions/workflows/build.yml)

# GEPT - [GE]neric [P]rogrammable [T]emplates

## Building

Build the program like this:

```bash
$ make clean && make
```

or simply:

```bash
$ gcc -Iinclude src/gept.c -o gept
```

## Usage

GEPT works in a similar way to the C preproccessor. It takes a template file
as input (via the `-i` or `--input` option) and produces the expanded version
on stdout. I.e. you would use gept like this:

```bash
$ ./gept --input my_file.c.template > myfile.c
```

A template file has no particular formatting requirements (hence "[GE]neric"),
except that it may contain one or more GEPT directives. Any line where the
first non-whitespace character is a `@` immediately followed by a GEPT keyword
indicates the start of a GEPT directive. There are two main classes of
directives, single-line and multi-line directives. Single line directives take
a set number of arguments. The end of a single line directive is indicated by
a `\n` newline character. Multi-line directives may span over multiple lines.
The end of a multi-line directive is indicated by a line containing only the
string "@end".

The following directives are supported:

- `@embed <file> [limit(N)]` \- the `@embed` directive is a single line-directive which
takes the path of a file as its argument and, upon
expansion, embeds it as a comma-separated (default) list of
byte-sized integers. Optionally, the `limit(N)` attribute
may be specified. The `limit(N)` attribute puts an upper
limit on the number of bytes to be embedded. This can be
useful when embedding data from "infinite length" files
such as /dev/urandom or any other device file with special
read semantics. The default `@embed` byte format and delimiter
may be changed using the `--embed-fmt` and `--embed-delim`
command-line options.
- `@include <file>`  \- the `@include` directive is a single line-directive which
works the same as the C preprocessor `#include` directive;
it will simply output the contents of <file>.
- `@sizeof <file>`   \- the `@sizeof` directive is a single line-directive which
takes the path of a file as its argument and expands to
the size of the file.
- `@bash ... @end`   \- the `@bash` directive is a multi-line directive, which
takes a bash script and expands to the output of said
bash script.
- `@python ... @end` \- the `@python` directive is a multi-line directive, which
takes a python-script and expands to the output of said
python script.
- `@perl ... @end`   \- the `@perl` directive is a multi-line directive, which
takes a perl-script and expands to the output of said
perl script.

By default, GEPT uses firejail to run subprocesses in a semi-sandboxed environment where
they can't make any changes to the file system (with a few exceptions, such as /tmp). This
is done to ensure that the @bash, @python, and @perl directives don't have any side effects
other than writing to stdout and to avoid possible footguns. Sandboxing can be disabled by 
running gept with the `--yolo` option which enables "YOLO-mode".

You can get a list of all supportet GEPT options by running `./gept --help`:

```
    GEPT - [GE]neric [P]rogrammable [T]emplates
    Usage: ./gept [Options]
    Options:
      -i,--input               Input file path (default = (null))
      --firejail-path          Alternative path to the firejail executable (default = (null))
      --python-path            Alternative path to the python3 executable (default = (null))
      --perl-path              Alternative path to the perl executable (default = (null))
      --bash-path              Alternative path to the bash executable (default = (null))
      --embed-fmt              C-style format string used by the @embed directive. (default = 0x%02X)
      --embed-delim            Delimiter string used by the @embed directive (default = , )
      -yolo, --yolo            Enable YOLO-mode. Run @python, @perl, and @bash in a non-sandboxed environment. (default = 0)
      -h,--help                Displays this help message (default = 0)
```


## Example

See the examples/ directory for an example template file.
