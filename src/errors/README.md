# The NRC error module
This is the code responsible for errors within NRC. It contains methods for throwing and printing errors. It also
contains the error index.

To use it, just include `<errors/error.h>`.

## The error index
The error index is a big JSON file containing a description of every error thrown by NRC. The structure is
as follows:
```json
{
  "$ Any line starting with an ampersand like this, is a comment": [],
  
  "$ 8 Example segment": [],
  "RCT8123": ["ERR_ERROR_ID", "Error description"]
}
```
So, not that hard right? The error id is going to turn into a macro so make sure it is a valid C identifier, prefixed
with `ERR_`. The description is currently unused but helps with readability.

Yes, it's a list and, yes, that's not good practice, but it works and is concise enough for this.

The file `error_index.h` is generated from `error_index.json`, and is just a bunch of defines as of right now for
you to use in your code.