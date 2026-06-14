# clox

A Bytecode Virtual Machine written in C for the Lox language from [Crafting Interpreters](https://craftinginterpreters.com/).

> **NOTE:** I did [one of the challenges](https://craftinginterpreters.com/chunks-of-bytecode.html#:~:text=Because%20OP_CONSTANT%20uses%20only%20a%20single%20byte%20for%20its%20operand%2C%20a%20chunk%20may%20only%20contain%20up%20to%20256%20different%20constants.%20That%E2%80%99s%20small%20enough%20that%20people%20writing%20real%2Dworld%20code%20will%20hit%20that%20limit.) that adds support for 3-byte long operands, that's why there are additional `xxx_LONG` instructions being emitted and handled throughout the VM.

## TODO:

Lox is a pretty small toy language missing some standard language features, this is more of learning project for me
so I think I'll only add the following:
- [ ] More arithmetic operators (pow/`**`, modulo/`%`)
- [ ] Jump statements (`break`, `continue`)
- [ ] Arrays

Other than that, I'm planning on adding some additional optimizations to the interpreter, here's what I'm thinking:

- [ ] Inline Caching for property access (i.e. for `OP_GET_PROPERTY`/`OP_SET_PROPERTY` - [apparently one of the instructions that take the longest amount of time](https://craftinginterpreters.com/optimization.html#:~:text=The%20big%20heavy%20instructions%20are%20OP_GET_GLOBAL%20with%2017%25%20of%20the%20execution%20time%2C%20OP_GET_PROPERTY%20at%2012%25%2C%20and%20OP_INVOKE%20which%20takes%20a%20whopping%2042%25%20of%20the%20total%20running%20time.))
- [ ] x86-64 Baseline JIT (compiling the instructions in `ObjFunction` to machine code when a certain call threshold is reached at runtime)