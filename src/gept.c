
/**
 * LICENSE:
 *
 * MIT License
 *
 * Copyright (c) 2024 Henrik A. Glass
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * MIT License
 *
 *
 * ABOUT:
 *
 * GEPT ([GE]neric [P]rogrammable [Templates]).
 *
 *
 * BUILDING:
 *
 * Build the program like this:
 *
 *     $ make clean && make
 *
 * or simply:
 *
 *     $ gcc -Iinclude src/gept.c -o gept
 *
 *
 * USAGE:
 *
 * GEPT works in a similar way to the C preproccessor. It takes a template file
 * as input (via the `-i` or `--input` option) and produces the expanded version
 * on stdout. I.e. you would use gept like this:
 *
 *     $ ./gept --input my_file.c.template > myfile.c
 *
 * A template file has no particular formatting requirements (hence "[GE]neric"),
 * except that it may contain one or more GEPT directives. Any line where the
 * first non-whitespace character is a `@` immediately followed by a GEPT keyword
 * indicates the start of a GEPT directive. There are two main classes of
 * directives, single-line and multi-line directives. Single line directives take
 * a set number of arguments. The end of a single line directive is indicated by
 * a `\n` newline character. Multi-line directives may span over multiple lines.
 * The end of a multi-line directive is indicated by a line containing only the
 * string "@end".
 *
 * The following directives are supported:
 *
 *     * @embed <file> [limit(N)] - the `@embed` directive is a single line-directive which
 *                                  takes the path of a file as its argument and, upon
 *                                  expansion, embeds it as a comma-separated (default) list of
 *                                  byte-sized integers. Optionally, the `limit(N)` attribute
 *                                  may be specified. The `limit(N)` attribute puts an upper
 *                                  limit on the number of bytes to be embedded. This can be
 *                                  useful when embedding data from "infinite length" files
 *                                  such as /dev/urandom or any other device file with special
 *                                  read semantics. The default @embed byte format and delimiter
 *                                  may be changed using the `--embed-fmt` and `--embed-delim`
 *                                  command-line options.
 *     * @include <file>          - the `@include` directive is a single line-directive which
 *                                  works the same as the C preprocessor `#include` directive;
 *                                  it will simply output the contents of <file>.
 *     * @sizeof <file>           - the `@sizeof` directive is a single line-directive which
 *                                  takes the path of a file as its argument and expands to
 *                                  the size of the file.
 *     * @bash ... @end           - the `@bash` directive is a multi-line directive, which
 *                                  takes a bash script and expands to the output of said
 *                                  bash script.
 *     * @python ... @end         - the `@python` directive is a multi-line directive, which
 *                                  takes a python-script and expands to the output of said
 *                                  python script.
 *     * @perl ... @end           - the `@perl` directive is a multi-line directive, which
 *                                  takes a perl-script and expands to the output of said
 *                                  perl script.
 *
 * By default, GEPT uses firejail to run subprocesses in a semi-sandboxed environment where
 * they can't make any changes to the file system (with a few exceptions, such as /tmp). This
 * is done to ensure that the @bash, @python, and @perl directives don't have any side effects
 * other than writing to stdout and to avoid possible footguns. Sandboxing can be disabled by 
 * running gept with the `--yolo` option which enables "YOLO-mode".
 *
 * You can get a list of all supportet GEPT options by running `./gept --help`:
 *
 *     GEPT - [GE]neric [P]rogrammable [T]emplates
 *     Usage: ./gept [Options]
 *     Options:
 *       -i,--input               Input file path (default = (null))
 *       --firejail-path          Alternative path to the firejail executable (default = (null))
 *       --python-path            Alternative path to the python3 executable (default = (null))
 *       --perl-path              Alternative path to the perl executable (default = (null))
 *       --bash-path              Alternative path to the bash executable (default = (null))
 *       --embed-fmt              C-style format string used by the @embed directive. (default = 0x%02X)
 *       --embed-delim            Delimiter string used by the @embed directive (default = , )
 *       -yolo, --yolo            Enable YOLO-mode. Run @python, @perl, and @bash in a non-sandboxed environment. (default = 0)
 *       -h,--help                Displays this help message (default = 0)
 * 
 *
 * EXAMPLE:
 *
 * See the examples/ directory for an example template file.
 *
 *
 * AUTHOR: Henrik A. Glass
 *
 */

#define _DEFAULT_SOURCE

#define HGL_FLAGS_IMPLEMENTATION
#include "hgl_flags.h"

#define HGL_STRING_IMPLEMENTATION
#include "hgl_string.h"

#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define GEPT_ASSERT(arg, ...)                     \
    if (!(arg)) {                                 \
        fprintf(stderr, "  ERROR: " __VA_ARGS__); \
        exit(1);                                  \
    }

#define GEPT_ASSERT_LINE(line, arg, ...)                                            \
    if (!(arg)) {                                                                   \
        fprintf(stderr, "  ERROR on line \"" HGL_SV_FMT "\"\n ", HGL_SV_ARG(line)); \
        fprintf(stderr, "  description: " __VA_ARGS__);                             \
        exit(1);                                                                    \
    }

#define SCRATCH_BUFFER_SIZE (128*1024*1024)

static const char **opt_infile;
static const char **opt_firejail_path;
static const char **opt_python_path;
static const char **opt_perl_path;
static const char **opt_bash_path;
static const char **opt_embed_fmt;
static const char **opt_embed_delim;
static bool        *opt_yolo;
static bool        *opt_help;

static uint8_t scratch_buf[SCRATCH_BUFFER_SIZE]; // 128 MiB should be enough for most things

int main(int argc, char *argv[])
{
    int err;

    /* parse cli arguments */
    opt_infile        = hgl_flags_add_str("-i,--input", "Input file path", NULL, 0);
    opt_firejail_path = hgl_flags_add_str("--firejail-path", "Alternative path to the firejail executable", NULL, 0);
    opt_python_path   = hgl_flags_add_str("--python-path", "Alternative path to the python3 executable", NULL, 0);
    opt_perl_path     = hgl_flags_add_str("--perl-path", "Alternative path to the perl executable", NULL, 0);
    opt_bash_path     = hgl_flags_add_str("--bash-path", "Alternative path to the bash executable", NULL, 0);
    opt_embed_fmt     = hgl_flags_add_str("--embed-fmt", "C-style format string used by the @embed directive.", "0x%02X", 0);
    opt_embed_delim   = hgl_flags_add_str("--embed-delim", "Delimiter string used by the @embed directive", ", ", 0);
    opt_yolo          = hgl_flags_add_bool("-yolo, --yolo", "Enable YOLO-mode. Run @python, @perl, and @bash in a non-sandboxed environment.", false, 0);
    opt_help          = hgl_flags_add_bool("-h,--help", "Displays this help message", false, 0);

    err = hgl_flags_parse(argc, argv);
    if (err != 0 || *opt_help || *opt_infile == NULL) {
        printf("GEPT - [GE]neric [P]rogrammable [T]emplates\n");
        printf("Usage: %s [Options]\n", argv[0]);
        hgl_flags_print();
        return 1;
    }

    int devnull = open("/dev/null", O_WRONLY);
    GEPT_ASSERT(devnull != - 1, "Unable to open /dev/null for writing.\n");

    /* Check that firejail is installed if not running in YOLO-mode. */
    if (!*opt_yolo) {
        pid_t pid = fork();

        /* ======== child ======== */
        if (pid == 0) {
            /* redirect stdout to the void */
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
            char *exec_argv[3];
            exec_argv[0] = "which";
            exec_argv[1] = (*opt_firejail_path == NULL) ? "firejail": *opt_firejail_path;
            exec_argv[2] = NULL;
#pragma GCC diagnostic pop

            /* on success `execve` doesn't return */
            if (-1 == execvp(exec_argv[0], exec_argv)) {
                fprintf(stderr, "ERROR (IN CHILD): failed to exec file. errno=%s\n",
                        strerror(errno));
                exit(1);
            }
            GEPT_ASSERT(0, "unreachable");
        }

        /* wait for process to terminate */
        pid_t wait_pid;
        int wstatus = 0;
        while ((wait_pid = waitpid(pid, &wstatus, 0)) != pid) {
            GEPT_ASSERT(wait_pid != -1, "waitpid() returned an error. errno=%s\n", strerror(errno));
        }
        if (WEXITSTATUS(wstatus) != 0) {
            fprintf(stderr, "Error: GEPT could not find a firejail executable installed on \n"); 
            fprintf(stderr, "       your system. Please make sure firejail is installed or, \n");
            fprintf(stderr, "       if you know what you're doing, run GEPT in \"YOLO mode\" \n");
            fprintf(stderr, "       by passing the `--yolo` option. \n");
            exit(1);
        }
    }

    /* open template file */
    HglStringView input;
    HglStringBuilder output = hgl_sb_make("", 4096);
    HglStringBuilder input_sb = hgl_sb_make("", 4096);
    err = hgl_sb_append_file(&input_sb, *opt_infile);
    GEPT_ASSERT(err == 0, "Call to `hgl_sb_append_file` failed.\n");
    input = hgl_sv_from_sb(&input_sb);

    /* generate output */
    HglStringView line;
    HglStringView tokens;

    while (input.length > 0) {

        /* get next line */
        line = hgl_sv_lchop_until(&input, '\n');
        tokens = hgl_sv_ltrim(line);

        /* regular code ==> append line to output */
        if (!hgl_sv_starts_with(&tokens, "@")) {
            hgl_sb_append_sv(&output, &line);
            hgl_sb_append_char(&output, '\n');
            continue;
        }

        HglStringView directive = hgl_sv_lchop_until(&tokens, ' ');

        /* @sizeof directive */
        if (hgl_sv_equals(directive, HGL_SV_LIT("@sizeof"))) {
            HglStringView path  = hgl_sv_lchop_until(&tokens, ' ');

            /* construct NULL-terminated path... */
            GEPT_ASSERT_LINE(line, path.length < 4096, "Path is too long");
            memcpy(scratch_buf, path.start, path.length);
            scratch_buf[path.length] = '\0';

            /* open file in read binary mode */
            int fd = open((char *)scratch_buf, O_RDWR);
            GEPT_ASSERT_LINE(line, fd != -1, "Call to `open` failed.");

            /* get file size */
            struct stat sb;
            err = fstat(fd, &sb);
            GEPT_ASSERT_LINE(line, err != -1, "Call to `fstat` failed.");

            /* append size to output */
            hgl_sb_append_fmt(&output, "    %zu", (size_t) sb.st_size);

            /* append remaining line to output */
            hgl_sb_append_char(&output, ' ');
            hgl_sb_append_sv(&output, &tokens);
            hgl_sb_append_char(&output, '\n');

            /* close file descriptor */
            close(fd);
        }

        /* @embed directive */
        if (hgl_sv_equals(directive, HGL_SV_LIT("@embed"))) {
            HglStringView path  = hgl_sv_lchop_until(&tokens, ' ');

            /* construct NULL-terminated path... */
            GEPT_ASSERT_LINE(line, path.length < 4096, "Path is too long");
            memcpy(scratch_buf, path.start, path.length);
            scratch_buf[path.length] = '\0';

            /* has limit(n) ? */
            int64_t limit = SCRATCH_BUFFER_SIZE;
            tokens = hgl_sv_ltrim(tokens);
            if (hgl_sv_starts_with_lchop(&tokens, "limit(")) {
                limit = (int64_t) hgl_sv_lchop_u64(&tokens);
                GEPT_ASSERT_LINE(line, hgl_sv_starts_with_lchop(&tokens, ")"), "Expected \')\'");
            }

            /* open file */
            FILE *fp = fopen((char *) scratch_buf, "rb");
            GEPT_ASSERT_LINE(line, fp != NULL, "Unable to open file `%s`", scratch_buf);

            /* Get file size if possible */
            fseek(fp, 0, SEEK_END);
            int64_t file_size = ftell(fp);
            rewind(fp);
            if (file_size <= 0) {
                file_size = limit;
            } else {
                file_size = (file_size < limit) ? file_size : limit;
            }

            /* read contents into scratch buffer */
            ssize_t n_read_bytes = fread(scratch_buf, 1, file_size, fp);
            (void) n_read_bytes;

            /* generate embedding as a list of 8-bit unsigned integers */
            hgl_sb_grow(&output, output.capacity + 6 * file_size); // probably enough
            const int64_t n_bytes_per_row = 20;
            const int64_t n_rows = file_size / n_bytes_per_row + 1;
            for (int64_t row = 0; row < n_rows; row++) {
                hgl_sb_append_cstr(&output, "    ");
                for (int64_t i = 0; i < n_bytes_per_row && row*n_bytes_per_row + i < file_size; i++) {
                    hgl_sb_append_fmt(&output, *opt_embed_fmt, scratch_buf[row * n_bytes_per_row + i]);
                    hgl_sb_append_cstr(&output, *opt_embed_delim);
                }
                hgl_sb_append_char(&output, '\n');
            }

            /* Remove last delimiter (typically `,`) */
            hgl_sb_rchop(&output, 1 + strlen(*opt_embed_delim));
            hgl_sb_append_char(&output, '\n');

            /* close file */
            fclose(fp);
        }

        /* @include directive */
        if (hgl_sv_equals(directive, HGL_SV_LIT("@include"))) {
            HglStringView path  = hgl_sv_lchop_until(&tokens, ' ');

            /* construct NULL-terminated path... */
            GEPT_ASSERT_LINE(line, path.length < 4096, "Path is too long");
            memcpy(scratch_buf, path.start, path.length);
            scratch_buf[path.length] = '\0';

            /* append file */
            hgl_sb_append_file(&output, (char *)scratch_buf);
        }

        /* @bash and @python directives */
        if (hgl_sv_equals(directive, HGL_SV_LIT("@bash")) ||
            hgl_sv_equals(directive, HGL_SV_LIT("@perl")) ||
            hgl_sv_equals(directive, HGL_SV_LIT("@python"))) {
            HglStringBuilder source_code = hgl_sb_make("", 4096);

            while (input.length > 0) {
                line = hgl_sv_lchop_until(&input, '\n');
                tokens = hgl_sv_ltrim(line);

                if (hgl_sv_starts_with(&tokens, "@end")) {
                    break;
                } else {
                    hgl_sb_append_sv(&source_code, &line);
                    hgl_sb_append_char(&source_code, '\n');
                }
            }

            GEPT_ASSERT(input.length > 0, "Missing terminating `@end` token for matching `"
                        HGL_SV_FMT"` token", HGL_SV_ARG(directive));

            int pipes[2][2]; // {{input read end, input write end},
                             //  {output read end, output write end}}
            GEPT_ASSERT(pipe(pipes[0]) == 0, "Failed to create pipes");
            GEPT_ASSERT(pipe(pipes[1]) == 0, "Failed to create pipes");

            pid_t pid = fork();

            /* ======== child ======== */
            if (pid == 0) {
                /* Replace stdin & stdout with respective pipe and close unused ends */
                close(pipes[0][1]);
                close(pipes[1][0]);
                dup2(pipes[0][0], STDIN_FILENO);
                dup2(pipes[1][1], STDOUT_FILENO);
                //dup2(devnull, STDERR_FILENO);

                char *exec_argv[32];
                int exec_argv_idx = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
                /* There is no reasonable const-correct way to do this*/

                /* 
                 * Run in a read-only view of the file system unless explicitly told "YOLO"
                 * by the user.
                 *
                 * TODO figure out what other flags to use.
                 */
                if (!*opt_yolo) {
                    exec_argv[exec_argv_idx++] = (*opt_firejail_path == NULL) ? "firejail": *opt_firejail_path;
                    exec_argv[exec_argv_idx++] = "--read-only=~/";
                    exec_argv[exec_argv_idx++] = "--caps.drop=all";
                    exec_argv[exec_argv_idx++] = "--protocol=netlink";
                    exec_argv[exec_argv_idx++] = "--quiet";
                }

                /* select which executable to run */
                if (hgl_sv_equals(directive, HGL_SV_LIT("@bash"))) {
                    exec_argv[exec_argv_idx++]= (*opt_bash_path == NULL) ? "bash" : *opt_bash_path;
                    exec_argv[exec_argv_idx++]= "-s";
                    exec_argv[exec_argv_idx++]= NULL;
                } else if (hgl_sv_equals(directive, HGL_SV_LIT("@python"))) {
                    exec_argv[exec_argv_idx++]= (*opt_python_path == NULL) ? "python3" : *opt_python_path;
                    exec_argv[exec_argv_idx++]= NULL;
                } else if (hgl_sv_equals(directive, HGL_SV_LIT("@perl"))) {
                    exec_argv[exec_argv_idx++]= (*opt_perl_path == NULL) ? "perl" : *opt_perl_path;
                    exec_argv[exec_argv_idx++]= NULL;
                }
#pragma GCC diagnostic pop

                /* on success `execve` doesn't return */
                if (-1 == execvp(exec_argv[0], exec_argv)) {
                    fprintf(stderr, "ERROR (IN CHILD): failed to exec file. errno=%s\n",
                            strerror(errno));
                    exit(1);
                }
                GEPT_ASSERT(0, "unreachable");
            }

            /* ======== parent ======== */

            /* close unused ends of pipes */
            close(pipes[0][0]);
            close(pipes[1][1]);

            /*
             * Write source code contents on the stdin of the process then
             * immediately close the pipe.
             */
            ssize_t n_written_bytes = write(pipes[0][1], source_code.cstr, source_code.length);
            (void) n_written_bytes;
            close(pipes[0][1]);

            /* wait for process to terminate */
            pid_t wait_pid;
            int wstatus = 0;
            while ((wait_pid = waitpid(pid, &wstatus, 0)) != pid) {
                GEPT_ASSERT(wait_pid != -1, "Child process returned an error");
            }
            GEPT_ASSERT(WEXITSTATUS(wstatus) == 0, "Child process exited with the error code: %d\n",
                        WEXITSTATUS(wstatus));


            /* read output, and close the pipe */
            ssize_t n_read_bytes = read(pipes[1][0], scratch_buf, sizeof(scratch_buf) - 1);
            close(pipes[1][0]);

            hgl_sb_append(&output, (char *)scratch_buf, n_read_bytes);
            hgl_sb_destroy(&source_code);
        }
    }

    /* print output to stdout */
    printf(HGL_SB_FMT "\n", HGL_SB_ARG(output));

    /* cleanup */
    hgl_sb_destroy(&input_sb);
    hgl_sb_destroy(&output);

    close(devnull);

    return 0;
}

// TODO: Better error messages. Especially when subprocesses fail.

