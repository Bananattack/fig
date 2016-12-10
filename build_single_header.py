#!/usr/bin/env python
import os
import os.path
import glob

# A little script to package all the c and h files in the library into a single file.

def strip_include_guard(include_guard_name, lines):
    ifndef_line_index = None
    define_line_index = None
    last_endif_line_index = None
    for line_index, line in enumerate(lines):
        stripped_line = line.strip()

        if ifndef_line_index is not None and define_line_index is None:
            if stripped_line.startswith('#define') and stripped_line.startswith('#define ' + include_guard_name):
                define_line_index = line_index
            else:
                raise Exception('expected `#define ' + include_guard_name + '` but got `' + stripped_line + '`')
        elif ifndef_line_index is None and stripped_line.startswith('#ifndef'):
            if stripped_line.startswith('#ifndef ' + include_guard_name):
                ifndef_line_index = line_index
            else:
                raise Exception('expected `#ifndef ' + include_guard_name + '` but got `' + stripped_line + '`')
        elif stripped_line.startswith('#endif'):
            last_endif_line_index = line_index

    if ifndef_line_index is None or define_line_index is None or last_endif_line_index is None:
        raise Exception('include guard is missing.')
    return lines[ifndef_line_index + 2 : last_endif_line_index]

def strip_header_include(header_filename, lines):
    return [line for line in lines if not line.strip().startswith('#include ' + header_filename)]

def ensure_ending_newline(lines):
    if len(lines) > 0 and not lines[-1].endswith('\n'):
        lines[-1] += '\n'
    return lines

def reorganize_includes(lines):
    include_lines = []
    normal_lines = []

    for line in lines:
        stripped_line = line.strip()
        if stripped_line.startswith('#include'):
            include_lines.append(stripped_line + '\n')
        else:
            normal_lines.append(line)

    return sorted(set(include_lines)) + normal_lines

fig_config_h_lines = ensure_ending_newline(strip_include_guard('FIG_CONFIG_H', list(open('include/fig_config.h'))))
fig_h_lines = ensure_ending_newline(strip_include_guard('FIG_H', list(open('include/fig.h'))))
fig_c_lines = []
for filename in sorted(glob.glob('src/*.c')):
    fig_c_lines.extend(ensure_ending_newline(strip_header_include('<fig.h>', list(open(filename)))))

fig_c_lines = reorganize_includes(fig_c_lines)

out_file = open('single_header/fig.h', 'w')
out_file.write('#ifndef FIG_H\n')
out_file.write('#define FIG_H\n')
out_file.write('/* A single-header distribution of fig. */\n')
out_file.write('/* To use, there must be ONE source file that contains the library implementation: */\n')
out_file.write('/* #define FIG_IMPLEMENTATION */\n')
out_file.write('/* #include <fig.h> */\n')
out_file.write(''.join(fig_config_h_lines))
out_file.write(''.join(fig_h_lines))
out_file.write('#ifdef FIG_IMPLEMENTATION\n')
out_file.write(''.join(fig_c_lines))
out_file.write('#endif\n\n')
out_file.write('#endif\n\n')


