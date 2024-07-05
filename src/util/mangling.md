# The NRC mangling schema

## Aim
In order to allow for easy comparisons, NRC provides name mangling.

The rules are:
- Each mangle is unique
- Each mangle should be replicable
- Each mangle should be able to restore the full object detals.

This means that if we have the mangle for `var myVariable -> SomeType;` we should be able to deduce:
- That the variable is called myVariable
- That the variable is of type SomeType. Not Types.SomeType or std::SomeType, but SomeType.

The mangle does not need to be reversible, but it needs to be predictable.
Type mangles should always be the full type.

## Structure of a mangle
Each mangle consists of a couple of "parts", depending on what the mangle is for.
The parts are prefaced with identifier code - this shows
if the mangle is a variable, class, struct, function etc.

Here is a table of mangle identifiers:

| Symbol | Type                   |
|--------|------------------------|
| $      | Type - class or struct |
| @      | Variables              |
| %      | Functions              |

Following the identifier code/symbol, you have the parts.
Each part contains a set of characters, and each set is separated by a hashtag character(`#`)