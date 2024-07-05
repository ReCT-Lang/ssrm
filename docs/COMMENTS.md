# Comments in NRC
Comments aren't really that special, so I'll keep it quick.

## Single-line comments
Comments that start with `//` are single-line. They go on until they
hit the end of the line, nothing more.

## Multi-line comments
Comments that start with `/*` go on until it hits the respective `*/`.
These can nest, so for example:
```
/* /* this is a valid comment */ */
```
But it wouldn't be in C, as it'd treat everything until the first `*/` as a comment
but then try to parse the second `*/` regularly.

## Pre-processor comments
Pre-proc comments are just fancy comments that the lexer uses to insert
stuff into source. They can also be used like regular comments(however it
is not recommended).

They're like in C, so they start with a `#`, and end at the newline, unless it is
preceded by a `\ `, in which case it'll keep on reading(ignoring the newline)