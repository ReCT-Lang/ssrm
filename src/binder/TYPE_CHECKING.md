# Type Checking
Time to figure out type checking.

So, how do we do this?

At compile time, we need to make sure that
an expression has the right type, and that the aforementioned
type has any functions we try to call on it.

So, as a basis, we use identifiers.

Now, we can generate a full name from ANY identifier in ANY
scope, and then use that, I'd imagine.
