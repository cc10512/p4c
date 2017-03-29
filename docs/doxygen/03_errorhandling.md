# Error handling

## Problem statement

The p4c compiler infrastructure supports negative testing, i.e., writing tests that are intended to
throw specific compiler errors. The errors are checked against a sample output.

The current implementation has the following limitations:
- the compiler stops at the first error encountered. This introduces a significant burden for
  negative testing since a separate test needs to be written to check each individual error, even
  in cases when the compiler could run though the entire source and report all possible errors.
- the same error is reported multiple times for the same context. For example ...
- the same error is reported in different ways throughout the program
- error and warning reporting level is not controllable through compiler options
- error messages are sometimes obscure and require internal compiler knowledge to resolve
- there are no hints on how the error can be fixed

Given these limitations, we propose to design an error reporting system that removes as many of the
above limitations as possible

## Solution

The design space to resolve the problem is large. LLVM and Clang have focused on error reporting
with exceptional success -- the error reporting is concise and meaningful, always pointing the
relationship between statements that cause errors and offering suggestions of how to fix the error.

The solution should at the least:

- strive to continue in the presence of errors as much as possible, such that multiple meanigful
  errors are reported. Add command line options to control erroneous behavior:
  - -Wstop-on-error/-Wno-stop-on-error to toggle
- classify the error types into fatal, errors and warnings, and identify classes of errors that can
  be controlled with options, for example:
  - -Werror to turn warnings into errors;
  - -Wno-category/-Wcategory to toggle certain classes of warnings and categories
- include an error catalogue that unifies messages
- keep track of reported errors to minimize message duplication
- link (and display) statements in the program that cause the errors. Outside syntax errors, most
  other errors are caused by different uses of programming constructs
- provide support for aggregating errors
- provide support for hinting to potential error fixes
