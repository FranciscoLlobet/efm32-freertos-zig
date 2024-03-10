# Contributing to Miso

To contribute to `miso` please:

- Fork the repository
- Create a new branch in your own repository
- Create a pull request to the `master` branch

Contributions must not be just *code*. It can be:

- documentation and improvements in documentation,
- new functionalities,
- fixes to existing functionalities,
- etc.

When raising a Pull-Request, the contributor acknowledges that:

- The contribution improves quality aspects of the project like:
  - functionality.
  - maintainability.
  - security.
- The contribution does not break existing functionality or it fixes issues reported or not.
- The contribution does not introduce conflicts related to licensing, patents or other legal aspects.
- The contribution does not introduce security vulnerabilities or is part of a security attack to any kind.
- The contribution adheres to the project licensing strategy.
- The contributor has the authorship or license to the contribution.
- The contributor allows the `miso` project to use this code contribution.
- New files contributed may include the contributor`s (author) name in the file header.

The repository owner or an appointed administrator decides a contribution is accepted or rejected.

A contribution may be rejected without notice. 

## Project Licenses

> [LICENSE.md](./LICENSE.md)

C code is licensed using a 3-Clause BSD License

Zig code is licensed using the MIT License

Python code is licensed using the MIT License

## Project repository

`miso` is hosted in Github at <https://github.com/FranciscoLlobet/efm32-freertos-zig/>.

## Project language

The main (human) communication language used in this project is English.

Code documentation must be in English. Write basic english words and short sentences.

## Programming languages

The main programming languages used in `miso` are:

- `zig`
- `C`

Tooling may include `python` 3.x

## Configuration languages

Configuration languages are

- Zon (Zig-Object Notation).
- JSON
- YAML

## Trunk-based development

`miso` follows a trunk based development strategy. Small and frequent changes are performed on a *feature* or *bugfix* branch and then merged to the repository trunk branch. Once the development on the branch is ready, it may be merged into the repository trunk using the Github pull request.

The trunk branch in this project is called `master`.

### Submodules, dependencies and 3rd party embedded code

Third-party code may be included in the development of `miso`. Depending on the availability, licensing and distribution strategy of the originating project, the code may be embedded into the repository (as, for example `FatFS` and the `CC3100 SDK`). It is preferred to use Git submodules to include external C-Code dependencies when possible.

The preferred methods are (in order of importance):

1. Zig packages
2. Git Submodules
3. Code embeddings

## Coding guidelines

// to do