/*
 * Copyright 2016 Eddie Antonio Santos <easantos@ualberta.ca>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>


/********************************** Types **********************************/
typedef int FileDescriptor;
typedef struct {
    const size_t length;
    const char *start;
} TextSegment;


/******************************** Constants ********************************/
#define FILE_NOT_OPENED ((FileDescriptor) -1)


/********************************* Globals *********************************/
static FileDescriptor urandom_fd = FILE_NOT_OPENED;
static const char *program_name;


/******************************** Functions  ********************************/
static void init_random();
__attribute__((noreturn)) static void usage_error();
static TextSegment read_file(const char *path);
static unsigned int count_lines(TextSegment segment);
static void randomize_lines(FILE* output, TextSegment text, long line_count);


/*********************************** main ***********************************/
int
main(int argc, char *argv[])
{
    program_name = argv[0];

    if (argc < 2) {
        usage_error("Must supply at least one filename");
    }

    init_random();

    TextSegment text = read_file(argv[1]);
    long line_count = count_lines(text);

    randomize_lines(stdout, text, line_count);

    return 0;
}


/******************** Random number generation utilities ********************/
static void
init_random()
{
    FILE *file = NULL;

    assert(urandom_fd == FILE_NOT_OPENED);

    file = fopen("/dev/urandom", "r");
    if (file == NULL) {
        perror("Could not open /def/urandom");
        exit(EXIT_FAILURE);
    }

    urandom_fd = fileno(file);
}

static size_t
urandom() {
    size_t result;
    int bytes_read;
    assert(urandom_fd != FILE_NOT_OPENED);

    bytes_read = read(urandom_fd, &result, sizeof(size_t));
    if (bytes_read != sizeof(size_t)) {
        perror("could not read from urandom");
        exit(EXIT_FAILURE);
    }

    return result;
}

static TextSegment*
random_line(TextSegment lines[], size_t count)
{
    assert(count > 0);
    return &lines[urandom() % count];
}

static void
swap_lines(TextSegment *a, TextSegment *b)
{
    TextSegment temporary;
    temporary = *b;

    *b = *a;
    *a = temporary;
}

static void
shuffle(TextSegment lines[], size_t count)
{
    /* Implements the Fisher-Yates shuffle. */

    for (size_t i = 0; i < (count - 1); i++) {
        TextSegment *this_line = &lines[i];
        /* Fetch random line from the rest. */
        swap_lines(this_line, random_line(this_line + 1, count - i - 1));
    }
}


/************************ Dealing with text segments ************************/
static TextSegment
read_file(const char *path)
{
    FILE *file;
    struct stat info;
    off_t length;
    void *mapping;

    file = fopen(path, "r");

    if (file == NULL) {
        fprintf(stderr, "Could not open %s", path);
        perror(NULL);
        exit(-1);
    }

    if (fstat(fileno(file), &info) < 0) {
        perror("Could not stat file");
    }

    length = info.st_size;

    if (length < 0) {
        return (TextSegment) {
            .length = 0,
            .start = NULL
        };
    }

    mapping = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fileno(file), 0);
    if (mapping == MAP_FAILED) {
        fprintf(stderr, "Could not map file %s", path);
        perror(NULL);
    }

    return (TextSegment) {
        .length = length,
        .start = mapping
    };
}


static unsigned int
count_lines(TextSegment segment)
{
    unsigned count = 0;
    const char *end = segment.start + segment.length;

    for (const char *c = segment.start; c < end; c++) {
        if (*c == '\n') {
            count++;
        }
    }

    /* Check if there is a trailing newline (there usually is). */
    if (end[-1] == '\n') {
        count--;
    }

    return count;
}

static long
populate_lines(TextSegment text, TextSegment lines[], long max_lines)
{
    size_t i;
    const char *c = text.start;
    const char *end = text.start + text.length;
    const char *line_start;

    for (i = 0; i < max_lines; i++) {
        line_start = c;
        /* Advance until the next line. */
        for (; c < end && *c != '\n'; c++)
            ;

        lines[i] = (TextSegment) {
            .start = line_start,
            .length = c - line_start
        };

        /* Skip over the newline. */
        c++;
    }

    return i;
}

static void
put_text_segment(FILE* output, TextSegment text)
{
    fwrite(text.start, text.length, 1, output);
}

static void
randomize_lines(FILE* output, TextSegment text, long line_count)
{
    TextSegment lines[line_count];

    /* Prepare the data structure. */
    long count = populate_lines(text, lines, line_count);
    assert(count == line_count);

    shuffle(lines , line_count);

    /* Print dem lines. */
    for (int i = 0; i < line_count; i++) {
        put_text_segment(output, lines[i]);
        putc('\n', output);
    }
}

/**************************** Usage and options ****************************/
 __attribute__((noreturn))
static void
usage_error(const char *string)
{
    if (string != NULL) {
        fprintf(stderr, "%s\n", string);
    }

    exit(-1);
}
