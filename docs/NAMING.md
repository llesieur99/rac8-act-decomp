# Naming the functions

A matching decompilation has to *earn* every name it uses: the executable
is stripped, so a name is a claim that has to be supported by the code, by
the callers, by the data the function touches, or by the module it belongs
to.  This document records the method that turns the retail build's own
leftovers into such names.

The names themselves stay in the contributor's workspace: they are the
game's, and the repository never carries game data.  What is published here
is the method and its measurements.

## What the retail build leaves behind

The target's executable is stripped of its symbol table, and its section
names are gone, but three things survive and each of them names things.

### The assertion messages

The build keeps the messages of its assertions, and an assertion names the
file, the line and usually the method it comes from:

```
<file>:<line> (<condition>)
```

The target carries **48 distinct source paths** and **53 assertions** in
that form, and they reveal the studio's own tree: the engine's shared
code, its physics module, its wad (archive) module, and the third-party
libraries it links — the sound system, the particle library, the
compression library, the job system.

The retail build keeps the *whole* message table of a module, and the
messages carry the method names with them, so one module's messages name
dozens of methods at once.

### The class names

The PlayStation 5's build of the same engine (the PC release, which is
the same code) carries its C++ type information, and the RTTI section
holds **14,085 class names**: 11,867 of the engine's own and 2,166 of
third-party libraries, spread over 46 namespaces.  Demangling them gives
the shape of the engine — its subsystems, its data types and the
relationships between them — which is what tells a function *what it
works on*.

### The strings themselves

**85,565 strings** are readable in the target's executable.  They are the
raw material of the naming: every one of them is a candidate for naming
the function that reads it.

## The bridge: from a string to a name

A string on its own names nothing; it is the *reference* to it that names
the function.  The chain is short and every step of it is checkable:

1. the global variable's address is reached through the TOC, and the
   compiler's own way of reaching it is a load from `r2`;
2. a data-flow analysis over the function follows the registers and finds
   the load that reads the string — **15,218 distinct strings** are
   reached this way in the target;
3. the load sits in a function, and the function's boundaries are known
   (see [CATALOGUE.md](CATALOGUE.md));
4. the string says which method, and the assertion says which file and
   line.

The target's own initialisation code shows how the TOC is reached, and the
global pointer is the one the module descriptor names:

```
li   r2, 0
oris r2, r2, 0xca
ori  r2, r2, 0x6b58
lwz  r2, 4(r2)      ; r2 = the module's global pointer
```

From there, a global is a signed 16-bit displacement from `r2`, and the
catalogue of the globals is in [GLOBALS.md](GLOBALS.md).

Three names were established this way in the target and checked end to
end — string, address, instruction, function, name — and the same
procedure applies to the module that owns the wad code, whose message
table names several of its methods at once.

## What the method gives

* every function that reads an assertion message has a **name**, a
  **source file** and a **line**;
* every function that reads a named global works on a **type**, which
  gives its parameters and its return value;
* the **module** a function belongs to follows from the descriptor, which
  is what gives a name to the functions that touch nothing but the code.

## What is still missing

* The names of the functions that read no string and touch no global: for
  them, the module they belong to and their callers are the only
  evidence, and the name has to stay a description of the place
  (`module_link_…`, `float_to_short_…` in `src/text/`).
* The third-party libraries' names come from their own sources, which
  are not the studio's and are not in the repository.
