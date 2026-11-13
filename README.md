# Hyperactive

The language is specified by the paper: [docs/Hyperactive_Final_Paper.pdf](docs/Hyperactive_Final_Paper.pdf).

## Setup

Requires a C compiler (gcc or clang), make and python3.

```
git clone https://github.com/avikj/hyperactivity.git
cd hyperactivity
./hyperactive install        # puts `hyperactive` on your PATH (~/.local/bin)
```

The machine (`build/cubical`) is built on first use and rebuilt when `src/` changes.

## Commands

```
hyperactive                      what is open and what can start now
hyperactive server               the task graph drawn as a graph at http://localhost:8080
hyperactive browse               every goal, ticket, definition, document and file, in a browser
hyperactive show NODE ...        a node or ticket: its text, what it serves, what serves it, its essential context
hyperactive tree                 the task graph from its root, as text
hyperactive order                the order of work: what can start now, then what waits on what
hyperactive open FILE:LINE NODE ... NAME TEXT   open a ticket at a site, as a child of NODEs, named by an English sentence
hyperactive close tN             a ticket resolved
hyperactive why NAME             why a declaration exists: its chain of reasons to the root
hyperactive unexplained [FILE]   declarations with no reason
hyperactive view TERM            a term over the task graph, evaluated by the machine
hyperactive check [FILE ...]     check files; with no file, the task graph
hyperactive repl                 the machine, interactive (#type e, #show e, #where name; end each block with a line holding '.')
hyperactive lsp                  the language server, for an editor
hyperactive library              check the library (lib/library.cub)
hyperactive brunerie             compute Brunerie's number (−2)
hyperactive every                check every file, each with its imports
hyperactive help                 this list
```
