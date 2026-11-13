# State, change and collaboration: how the work is held, changed and shared, and why

Avik's design, from his messages to Claude sessions on 2026-11-13, quoted verbatim. It governs how
agents and Avik interface with the language and its work: the interface (`how-people-and-agents-read-write-and-act-on-the-work` in the task
graph) and the work object (`all-work-and-all-knowledge-about-it-as-one-content-addressed-object-in-the-language`) are built on it. Read it before working on either, and
before proposing anything about storage, versioning, collaboration or ownership.

## 1. Our state is not text

> First things first our state is not text. We may treat certain bodies as strings with no further
> semantics/syntax. but ultimately I should always think of this as a data structure, and really
> there is zero primitive filesystem concept in that data structure - yet filesystem data is
> trivially expressed within it.

> drop the idea of a repo filesystem on disc. that could be data that backs/drives the interface but
> in our represented world which is interfaced with, no file tree, only a collection of objects
> relations etc. files are largely organizational so we open up the organization space & make it
> malleable.

On non-cubical content (C source and the like):

> Zero splitting of non-cubical code.

> C semantics formalization is a ways away, actually that's whole answer.

## 2. A work session: read, write, act

> We do need for any given work session, contents on disk for what they intend to execute on-disk
> (forget cloud execution for now) - but for an arbitrary read/write loop on files, this can be
> driven entirely via mcp/api server. agent is already operating via a query interface over the
> filesystem ontology.

> ok so why do we need things on this machine? what do we need on this machine and when?
>
> to run something: it's executable+input
>
> that could be running a build, execution, interpretation, type-check, manipulation/inspection of
> an artifact via a script/command you own locally (grep/sed in a text file can be local & require
> materialization, so we don't have to rebuild all these utilities in our protocol nor allow remote
> execution of commands; our protocol is sort of already defined and purely minimal, happy to flesh
> out details )
>
> materialize it locally if I need to act on it locally
>
> cause that's all you can do; read, write, act, already entirely through command interface this is
> fucking clean.
>
> then on the other (server) side, basically maintain all data needed to support all agents' views

> every write action an agent takes (recall the entire workflow including thinking raising
> questions, tracking questions & resolutions) at high frequency hyperactively

## 3. Visibility

> there are things you Claude as a working agent should see, and things I as human owner should see.
> plenty of what you do should be internal to you; plenty of the words you produce, code you write,
> should never reach me.
>
> so there's a sense that there is something internally in-progress, which should be externally
> invisible

## 4. There is no canonical state

> You have to accept the reality there is no canonical state. You may need to repeat it 10 times.
>
> In a well run technology operation of highly intelligent collaborators - there is no canonical
> state. it's a lie and it slows things down and is the source of confusion
>
> there is no canonical set of patches; no mechanism of promoting an incoming patch to canonical. any
> incoming change is compatible with all existing code: it is new code.

Said to an agent that proposed a canonical view plus each agent's additions:

> Ahh, I see!! You solved it!! We have a main canonical state and each agent has their diff against
> it!! and we can name our system git!!
>
> I raised that shit exactly to fucking diffuse this notion.

## 5. Code is immutable and content-addressed

> oh wait.. we had immutable, content-addressed code as a starting point - two agents fundamentally
> cannot be working on the same thing. There is no sense of a file mutated over time. Two agents are
> working on extending the same thing at the same time, either producing identical or distinct new
> definitions over the old one. Both extensions live in the codebase. they are simply different
> function, two people named the same way and intended to serve the same purpose; any calling code
> must specify the exact function by hash not name, so there's is no ambiguity; only alternatives

> if I "change" some function f called within g.
>
> I have changed g.
>
> if you intend to "change" all call-sites of a function, really you emit new functions "replacing"
> everything that called the old ones.
>
> note an entire program, hell an entire distributed system, any arbitrary construction, is
> referenced by single hash.

> There isn't a program which changed, when you changed some implementation detail.
>
> The binary is a different program. Every call site now implements a different program. In a
> mutable codebase when you "change" a function, anything depending on it no longer exists; and all
> dependent programs (functions which call it) specialize to new programs with new behavior compared
> to their prior behavior.
>
> The idea of a persistent thing (the class, file name) being mutated by code edits is a harmful
> illusion. But the intention may be valid.. though im pretty sure release management etc build
> systems increasingly towards the system I describe (you don't say I depend on a binary with this
> name, and accept any binary with that name - a given release of a multi-component software system
> is determined by exact identity of each component.

> It's been years since upon unison & has been central to all thinking/work across theory/ux of
> interaction & languages ; they really fucking cracked this shit and we're cracking some next level
> shit beyond that . 99% of swes haven't heard of unison but we know it perfectly well understand it
> and the continuation perfectly well.

> We aren't thinning of collections of patches. Fuck patch theory that shit is much less retarded
> than git but still retarded relative to immutable content addressing / the correct organization of
> information.

## 6. Old objects, and marking them dead

> Ok so I change some deep called function. Now everything that calls it - I have a new object
> rendered calling the new function instead. What happens to the old objects?
>
> Won't we just accumulate?
>
> Responsibility to mark dead;
>
> ie if all you do is define the new versions, your essentially stating "here's another system"
>
> If you want to change a behavior system-wide, the statement must be "here's another system doing X
> correctly, and which is incorrect in the prior system; the prior system is undesirable and I throw
> it in the trash in favor of this one.

## 7. Change is a square

> oh fuck the actual dependent type theory hits the fucking spot here
>
> when we change f->f', and we have a g which calls f, and we want a g' just like g but instead of f
> it calls f'; g/g' dependent functions on f/f'.
>
> we don't just generate a new call-graph with new random hashes for all nodes now have a bunch of
> functions identified by hash. f, f', g, g' are related by a square. this models every case where a
> construction is invoked inside another construction, and we ask of the outer construction
> corresponding to a change in the inner construction. all that nesting/mutation falls to this.
> everything is connected

## 8. Questions are tasks

> oh also - a question needs a reason for even being asked, our work/goal system should cover this
>
> A question is a local subgoal about management of one's own uncertainty on information relevant to
> a higher goal

## 9. Ownership

> Also this "who" concept is weird, I'm the only human, my agents are all named claude and
> effectively have no persistent identity beyond a conversation. it should just indicate if it's
> owned probably.

The ownership rules worked out with Avik in an earlier session, as that session's commit 94e615a
implemented them (dropped in a merge on 2026-11-13, to be rebuilt on the design above rather than on
git commits): a leaf has at most one owner; each active collaborator owns one leaf; a new agent's
briefing is the unowned leaves that can start, in the order of work; opening a node under an owned
leaf X creates the new node and X′, the rest of X, and moves X's ownership to X′.

## 10. Priorities

> Do not manufacture bullshit work.

> I give zero fucks about C. I'm building hyperactive; this UX stuff is directly core hyperactive
> infra - how to humans & agents interface with the language.

> concretely, i have now decided that I do not plan on ever opening a text editor or IDE again.
> assume no such thing even exists.

> this is a masterwork of malleable software. i.e. beyond the dreams of ink&switch. the endgame of a
> global operating system of such.

> This hci vision that this PL work grounds has always been intended for interactive collaborative
> systems
