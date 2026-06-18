# clox

A Bytecode Virtual Machine written in C for the Lox language from [Crafting Interpreters](https://craftinginterpreters.com/).

> **NOTE:** I did [one of the challenges](https://craftinginterpreters.com/chunks-of-bytecode.html#:~:text=Because%20OP_CONSTANT%20uses%20only%20a%20single%20byte%20for%20its%20operand%2C%20a%20chunk%20may%20only%20contain%20up%20to%20256%20different%20constants.%20That%E2%80%99s%20small%20enough%20that%20people%20writing%20real%2Dworld%20code%20will%20hit%20that%20limit.) that adds support for 3-byte long operands, that's why there are additional `xxx_LONG` instructions being emitted and handled throughout the VM.

## TODO:

Lox is a pretty small toy language missing some standard language features, this is more of learning project for me
so I think I'll only add the following:
- [X] Multi-line comments
- [X] More arithmetic operators (pow/`**`, modulo/`%`)
- [X] Jump statements (`break`, `continue`)
- [X] Arrays
    - [X] Initializing arrays
    - [X] Printing arrays
    - [X] Indexing into arrays
    - [X] Assigning at array indices
- [X] String indexing
    - **NOTE:** Because strings are interned, I'm only adding indexing for get operations.
- [ ] Type methods
    <details>
    <summary>Array methods (<code>arr.push()</code>, <code>arr.pop()</code>, <code>arr.len()</code>, etc.)</summary>

    - [X] `arr.len()`
    - [X] `arr.push(val)`
    - [X] `arr.pop()`
    - [X] `arr.get(i)`
    - [X] `arr.set(i, val)`
    - [X] `arr.insert(i, val)`
    - [X] `arr.remove(i)`
    - [X] `arr.contains(val)`
    - [X] `arr.indexOf(val)`
    - [X] `arr.slice(a, b=n)`
    - [X] `arr.concat(other)`
    - [X] `arr.reverse()`
    - [X] `arr.join(sep)`
    - [X] `arr.clear()`
    - [X] `arr.isEmpty()`
    - [X] `arr.copy()` (shallow copy)

    </details>
    <details>
    <summary>String methods (<code>str.len()</code>, <code>str.replace()</code>, etc.)</summary>

    - [X] `str.len()`
    - [X] `str.replace(old, new)`
    - [X] `str.contains(sub)`
    - [X] `str.startsWith(pre)`
    - [X] `str.endsWith(suf)`
    - [X] `str.indexOf(sub)`
    - [X] `str.slice(a, b)`
    - [ ] `str.toUpper()`
    - [ ] `str.toLower()`
    - [ ] `str.trim()`
    - [ ] `str.trimStart()`
    - [ ] `str.trimEnd()`
    - [X] `str.split(sep)`
    - [X] `str.isEmpty()`
    - [ ] `str.repeat(n)`

    </details>

Other than that, I'm planning on adding some additional optimizations to the interpreter, here's what I'm thinking:

- [ ] Inline Caching for property access (i.e. for `OP_GET_PROPERTY`/`OP_SET_PROPERTY` - [apparently one of the instructions that take the longest amount of time](https://craftinginterpreters.com/optimization.html#:~:text=The%20big%20heavy%20instructions%20are%20OP_GET_GLOBAL%20with%2017%25%20of%20the%20execution%20time%2C%20OP_GET_PROPERTY%20at%2012%25%2C%20and%20OP_INVOKE%20which%20takes%20a%20whopping%2042%25%20of%20the%20total%20running%20time.))
- [ ] x86-64 Baseline JIT (compiling the instructions in `ObjFunction` to machine code when a certain call threshold is reached at runtime)