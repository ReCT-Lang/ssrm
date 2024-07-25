// Error index header
// Auto generated! Don't edit!


//  This is a comment!


//  0 - Internal errors

#define ERR_UNKNOWN ("RCT0000") // Something went wrong within nrc! Report!
#define ERR_RESOLVER_MISSING ("RCT0300") // The binder could not find a package resolver!

//  1 - Lexer errors

#define ERR_INVALID_CHAR ("RCT1010") // An invalid character was found!
#define ERR_UNEXPECTED_EOF ("RCT1011") // An unexpected EOF was found!
#define ERR_UNEXPECTED_NL ("RCT1012") // An unexpected newline was found!

//  2 - Parser errors

#define ERR_TOKEN_UNEXPECTED ("RCT2001") // An unexpected token was found!
#define ERR_TOKEN_UNEXPECTED_B ("RCT2002") // A token other than what was expected was found!

//  3 - Binder errors

#define ERR_NODE_UNEXPECTED ("RCT3001") // An unexpected node was found!
#define ERR_DECL_DISALLOWED ("RCT3002") // A declaration was not allowed within the current block!
#define ERR_ACCESS_DUALITY ("RCT3010") // A declaration cannot be public and private at the same time!
#define ERR_PACKAGE_NOTFOUND ("RCT3020") // A package couldn't be found!
