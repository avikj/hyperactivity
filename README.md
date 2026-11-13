# Hyperactive

The language is specified by the paper: [docs/Hyperactive_Final_Paper.pdf](docs/Hyperactive_Final_Paper.pdf).
Everything else, from what is implemented today to what it is for: [HYPERACTIVE.md](HYPERACTIVE.md).

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
hyperactive show NODE ...        a node: its statement, what it serves, what serves it, its essential context
hyperactive tree                 the task graph from its root, as text
hyperactive order                the order of work: what can start now, then what waits on what
hyperactive why NAME             why a declaration exists: its chain of reasons to the root
hyperactive unexplained [FILE]   declarations with no reason
hyperactive view TERM            a term over the task graph, evaluated by the machine
hyperactive check [FILE ...]     check files; with no file, the task graph
hyperactive repl                 the machine, interactive (#type e, #show e, #where name; end each block with a line holding '.')
hyperactive library              check the library (lib/library.cub)
hyperactive brunerie             compute Brunerie's number (−2)
hyperactive every                check every file, each with its imports
hyperactive help                 this list
```
