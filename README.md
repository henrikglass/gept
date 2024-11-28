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

- `@embed <file>`    \- the `@embed` directive is a single line-directive which
takes the path of a file as its argument and, upon
expansion, embeds it as a comma-separated list of
byte-sized integers.
- `@sizeof <file>`   \- the `@sizeof` directive is a single line-directive which
takes the path of a file as its argument and expands to
the size of the file.
- `@bash ... @end`   \- the `@bash` directive is a multi-line directive, which
takes a bash script and expands to the output of said
bash script.
- `@python ... @end` \- the `@python` directive is a multi-line directive, which
takes a python-script and expands to the output of said
python script.

You can get a list of all supportet GEPT options by running `./gept --help`:

```
GEPT - [GE]neric [P]rogrammable [T]emplates
Usage: ./gept [Options]
Options:
-i,--input               Input file path (default = (null))
--python-path            Path to the python3 executable (default = /usr/bin/python3)
--bash-path              Path to the bash executable (default = /bin/bash)
-h,--help                Displays this help message (default = 0)
```

## Example

See the examples/ directory for an example template file.
