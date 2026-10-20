# src

The accepted authored C/C++ definitions, one per function.

Nothing lives here yet: the function catalogue is the first milestone
(`docs/ROADMAP.md`, step 2), and a function only reaches this directory
after the body gate and the placement gate pass.

When a file lands here it holds:

* one function, and nothing else;
* the behaviour of the retail function, including its bugs;
* the types and the names the evidence supports, with the alias marked as
  an alias;
* no assembly, no inline assembly, no fixed-register bindings, no raw
  instruction words.

The layout follows the executable's own structure once the catalogue
exists: the module or the area a function belongs to becomes a directory,
and the file is named after the function.
