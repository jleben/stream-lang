## Prerequisits

This project requires flexc++ and bisonc++ lexer and parser generators.
Information about these two programs is available online:
- http://flexcpp.sourceforge.net/
- http://bisoncpp.sourceforge.net/

## Building

This project provides a CMake build system.

The standard building procedure is:
```
mkdir build
cd build
cmake ..
make
```

This will produce the following executables:
- `build/frontend/test` - Test the frontend (lexer + parser) on an input file.

CMake options:
- `FRONTEND_PRINT_TOKENS` - Compile frontend so that it prints all tokens on command-line.

## Usage

Currently, the only executable is the compiler frontend tester, built by
the above procedure at `build/frontend/test`. It may be invoked so:

```
test <input-file> [function-name [function-argument ...]]
```

The meaning of arguments:
- `input-file`: Require name of file with code to be compiled.
- `function-name`: Name of function to perform semantic analysis on.
- `function-argument`: One or more arguments to `function-name`.
  The arguments can be one of the following examplary literary forms:
  - `"[5, 6, 10]"`: A stream with size in each dimension represented by one
    of the integers.
  - `123` An integer number constant.
  - `123.45` A real number constant.

Example code files are provided in the `examples` folder. For example,
the matrix-multiply.in can be syntactically and semantically processed
with the following command:
```
build/frontend/test examples/matrix-multiply matrix_multiply "[10,3,5]" "[10,5,8]"
```

The program will print the following information on standard output:

- Each token produced by the lexical scanner (if the `FRONTEND_PRINT_TOKENS`
  CMake option was enabled)

- The abstract syntax tree (AST).

- The symbols (functions and constants) declared at the top level and stored
  into the global symbol table.

- The type of result of evaluation of the function and arguments given on
  command line.

## Filesystem:

- `frontend` - Contains code for the frontend (lexer + parser).
  - `scanner.l` - Input file for lexer generator flexc++
  - `parser.y` - Input file for parser generator bisonc++
  - `ast.hpp` - Abstract Syntax Tree (AST) representation
  - `ast_printer.hpp` - AST printing
  - `semantic.hpp`, `semantic.cpp` - Semantic analysis (type-checking, etc.)
  - `test.cpp` - An executable parser which depends on output of flexc++ and bisonc++.

- `examples` - Contains example code in the language implemented by this project.
  - `matrix-mult.in` - Implements multiplication of two sequences of matrices.
  - `autocorrelation.in` - Implements autocorrelation of a 1D sequence.
  - `spectral-flux.in` - Implements "spectral flux", given a sequence of spectrums (results of DFT).

## Notes

This project includes output files of the lexer and parser generators.
The reason is that the functionality of the flexc++ and bisonc++ input files
is smaller than that of the mainstream flex and bison.
In contrast, part of that functionality may be achieved by modification of
*some* files generated by flexc++ and bisonc++ which are only generated the
first time (if they don't exist) and otherwise not overridden.
This is the approach intended and suggested by the authors of flexc++ and
bisonc++.

## Online repository:

The entire project code is available online at:
https://github.com/jleben/stream-lang

## Author:

Jakob Leben
