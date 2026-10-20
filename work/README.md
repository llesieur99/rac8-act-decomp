# work

The private workspace. Everything here is git-ignored, and nothing here is
published.

| Path | What it holds |
|---|---|
| `pkg/` | the package and the files extracted from it |
| `eboot/` | the decrypted executable and the rebuilt ELF |
| `analysis/` | the disassembler's project and its exports |
| `lots/` | the per-agent working directories |
| `gate/` | the gate's scratch output and its receipts |

The rules are the same as everywhere else in the project: the workspace
holds the game-derived material, the repository holds the written result.
A lot directory is temporary; what matters is what the pull request says.

`work/templates/` holds the templates an agent starts from.
