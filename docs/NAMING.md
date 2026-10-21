# Naming the functions

A matching decompilation has to *earn* every name it uses: the executable
is stripped, so a name is a claim that has to be supported by the code, by
the callers, by the data the function touches, or by the module it belongs
to.  This document records the method that turns the retail build's own
leftovers into such names, and what it actually reaches.

The names themselves stay in the contributor's workspace: they are the
game's, and the repository never carries game data.  What is published
here is the method, its corrections and its measurements.

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
libraries it links.

The retail build keeps the *whole* message table of a module, and the
messages carry the method names with them, so one module's messages name
dozens of methods at once.

### The class names

The PlayStation 5's build of the same engine carries its C++ type
information, and the RTTI section holds **14,085 class names**.  They
describe the *newer* engine, though, and two measurements say how far they
can be carried over — see *A correction* below.

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
   the load that reads the string;
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

## Seven mistakes the model made

The first version of the chain reached 15,218 strings.  The second reached
**18,900**, and the difference is seven real mistakes in the model of the
PowerPC, each of them checked instruction by instruction against the
disassembler:

1. the **logical group** (`or`, `and`, `xor`) has its destination in bits
   16-20, not in 21-25 — the decoder was writing to the wrong register;
2. a **write to memory modifies no register**: the prototype erased `r30`
   in the middle of a function, and with it the copy of the TOC base;
3. the catalogue of the prologue and call scans contains **pseudo
   functions**; used as a boundary it reset the registers in the middle of
   a real function;
4. a **branch modifies no register**, and the prototype lost the line
   number of the assertions with it;
5. the **read-only data is inside the code segment**, so a pointer to a
   string was taken for a pointer to a function;
6. a value can be a string *and* a block of data — the string must not
   be cut;
7. the catalogue of strings keeps only the strings of four characters or
   more.

Those seven are worth more than the names they produced: they are the
difference between a plausible model and the machine.

## The campaign

The chain was run over **every** function of the catalogue — 34,482 of them
— and every function got a sheet: a name in the form `Module::method`, a
level of proof, and the reason for it.  The naming rule is deterministic,
in twelve steps, and eleven kinds of proof feed it (the string a function
reads, an assertion, a class, a named global, a dispatch table, a format
string, a source file, a linked module, the link unit, the neighbours, the
callers).

The coherence control applies **thirteen rules** automatically: a name has
to agree with the module, with the type of the global, with the class and
with the callers.  No sheet was blocked, and the doubtful ones are kept
apart with a status rather than dropped silently.

## The coverage, measured

Over the **34,482** procedures the catalogue now holds:

| | Count | Share |
|---|---:|---:|
| the name is **proved** (a message, an assertion, a class, a named global) | 236 | 0.7 % |
| the name is **deducible** (the structure it works on) | 16,518 | 47.9 % |
| the name is **described** | 34,482 | 100 % |
| the name is its own (not a fallback) | 1,439 | 4.2 % |

The second line is the one that matters for the matching: **half of the
program's functions have a signature that can be deduced** from the
globals they touch, and a signature is what a candidate needs.

## Two limits worth knowing

**The dispatch tables are filled at run time.**  74 of them were found, 24
of them used, 114 indirect call sites, 398 functions — and **not one method
can be named from them**, because the table is filled when the program
starts.  That is a hard limit: the static analysis can say that a
function is called through a table, and which table, but not which method
it is.  It is better to say so than to invent a name.

**17,963 functions — 52 % of the program — have no proof at all**: no
string, no named global, no module.  They carry a fallback name, and the
project's rule says so: a descriptive name is a name with a stated
reason.

## The link unit is a neighbourhood

The unit the descriptor names is not a translation unit: it is a
*neighbourhood* — a maximal block of 0x400 bytes, extended to 0x2000 when
the functions are contiguous.  Past eight functions, a unit is only named
by a strong proof or by two agreeing voices.  That stricter measure
removed a false attribution the laxer one had made: the module's
initialisation function had been named after an isolated credit mention.

## The index

The 46 modules are indexed with their classes, their named globals and
their source files, and the functions are distributed over them: the
third-party libraries take the largest shares, and a fifth of the program
belongs to no module at all — the core of the engine, which the compiler
put outside every module.

## A correction

The class names of the PlayStation 5's build are **not** a dictionary for
the PlayStation 3's.  Of the 9,511 entities the PS3 binary names, only
**three** exist in the newer build's RTTI: the two are a generation apart,
and the names moved.  The eight class names the PS3 build prints itself
do not exist in the newer vocabulary at all.

The newer type information can still *constrain* the older structures — it
carries the virtual functions, the vtable offset and the base classes —
but it cannot name them, and it does not even carry their size: the
mangled name's prefix is a reference marker, not a size, and the
metadata holds no size field.

So the names come from the build itself: the messages it prints, the
globals it names and the modules it links.  The newer build's type
information is a constraint on that, not a source of names.

## The publishable form

The evidence sheets can be published without the game's data: an address,
a size, a level of proof, the *kinds* of proof, the types of the globals
and the counts of the calls — no string, no class name, no module name.
That form is what the repository would take, if the project ever wants to
publish what it knows about the program's shape.

## What is still missing

* The names of the functions that read no string and touch no global: for
  them, the module they belong to and their callers are the only
  evidence, and the name has to stay a description of the place
  (`module_link_…`, `float_to_short_…` in `src/text/`).
* The names of the methods reached through a dispatch table: the table is
  filled at run time, so the method is not in the binary.
* The third-party libraries' names come from their own sources, which are
  not the studio's and are not in the repository.
