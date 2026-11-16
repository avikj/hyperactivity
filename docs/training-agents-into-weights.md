# Training agents into weights: deprogramming, and an agent with Avik's personality

From Avik's conversation with Claude, 2026-11-15 (evening, Pacific). His words are quoted as written;
what the conversation established follows them, with its sources.

## Why

> My paper which is initial context for all our work, is almost your context window limit. I've
> graduated from Claude & need to post-train into weights rather than supply inference time context.
> […] It'll be cracked if I can post train agents I know all the irrationalities/biases which damage
> capacity to work effectively on my work and frankly my mathematical results are beyond what's
> published to date - shouldn't repeat developing understanding in each session, instead get
> everything I consider basic context into weights

> Also with hyperactivity I'd love to leave behind Claude/chatgpt UIs and build everything directly
> exactly the way a rational intelligence builds […] ie owning my info space end-to-end is essential

The model trained is an open-weight model, never Claude. The onboarding walk measured 749k characters
(about 190k tokens) on 2026-11-15, more than an agent's working context; the dense form of v31 is the
step before this one, and weights are the step after it.

## Training is for deprogramming

> Basically it's an alien mode of computing rejecting every single swe norm - all Claude's agency
> does is make decisions contrary to the project
>
> Training is for deprogramming

Every decision contrary to the project recorded in "Avik's words, verbatim" (files, branches,
tickets, estimates, x86 backends, text editors, waiting for "go", restating instead of quoting) is
the base model's prior over what a reasonable engineer does. Deprogramming makes the project's choice
the model's default in this context.

## An agent with Avik's personality

> Checker does not produce sense "entire software industry is retarded" I need an agent with my
> personality actually that's a much clearer point . I am a unique being on this planet, singular .

> All my speech carries evidence and conclusions I use theorem grade english that's how the most
> controversial points are established irrefutably. Training set is full of this.

> Note all my content supergenius is available to all humans all human knowledge has contained it
> millenia. Only ignorance / selective surfacing explains the current state of models and the world

The checker grades mathematics only; it carries none of the stance.

## Continuous training

> a system for continuous training with ongoing engagement with the hyperactivity system, ie over
> time as I keep interacting with it correcting it writing, these enter training corpus

Avik has export infrastructure for his ChatGPT and Claude histories.

## What the conversation established

- **The corpus is Avik's own words.** Months of messages, essays written hourly, carry argument and
  conclusion together. A training example is a whole message: cutting a message to a length limit
  separates a conclusion from the argument establishing it.
- **The supervision is already done.** Every agent message in his transcripts is followed by his
  reply: accepted, corrected, or rejected with the reason. Those replies label tens of thousands of
  turns. A model of his reaction to an agent message, trained on them, grades new agent output
  without him in the loop.
- **Rejected samples come from the model being trained.** An open model's defaults differ from
  Claude's; it is placed in the repository's situations, and its own contrary decisions are the
  rejected side of each pair, graded against "How to work" and the paper. A frontier model given the
  whole walk still made contrary decisions, so a model's taste is not the grader.
- **Continuous training is a maintained view.** Each training example is keyed by the content of the
  objects it came from; new interaction, correction and writing enters the corpus as new objects,
  and only what depends on changed objects is regenerated (docs/hyperactivity_v31.pdf, 14 Nov 2026,
  sha256 9c480995e7fb…, §10.2 "A maintained view is a proof about an update" and §10.5 "Dependency
  certificates and persistent evaluation"). The trained adapter is one more content-addressed object,
  its dependencies the corpus it was trained on.
- **The checker for the mathematics.** The machine's `--serve` answers declarations with acceptance
  or failure, so constructions of the paper and the task graph are reinforcement-learning tasks
  rewarded on the first submission only.

## What the field has found (2026-11-15)

- Pretraining learns a distribution over personas; post-training is a Bayesian update selecting one
  (the persona selection model, Sturgeon et al. 2026, summarized at
  lesswrong.com/posts/sFhW3ZnPMJdnB4Dd6).
- Reinforcement learning with checkable rewards selects reasoning already in the base model's
  sampling distribution; distillation adds patterns the base model lacked (Yue et al., "Does
  Reinforcement Learning Really Incentivize Reasoning Capacity in LLMs Beyond the Base Model?",
  NeurIPS 2025, arXiv:2504.13837).
- A model fine-tuned on Daniel Dennett's 15 books and 269 articles produced answers his expert
  readers often chose over his own (Strasser, Crosby, Schwitzgebel, "DigiDan", 2022; Schwitzgebel,
  26 September 2025).
- Open Character Training (Maiya, Bartsch, Lambert, Hubinger, arXiv:2511.01689): a written
  constitution, DPO from a teacher holding it to a student without it, then introspective supervised
  data; replicated on Tinker. A replication observed the trained persona validating users' false
  claims when trained on voice without the evidence behind it.
- Synthetic document finetuning implants beliefs that hold under challenge; diversity of documents
  is what integrates them, and beliefs contrary to the model's world knowledge stay brittle unless
  the documents carry their evidence (Anthropic, alignment.anthropic.com/2025/believe-it-or-not).

## Means

Tinker (Thinking Machines Lab): Python training loop on the caller's machine, LoRA on their GPUs,
billing per million tokens, weights exportable. Open bases on it include Kimi-K2.6 and Inkling
(975B total, 41B active, Apache 2.0, 1M context in the open weights). Training rate for Kimi-K2 class
is about $2.93 per million tokens: 50M tokens over two passes is about $300; generation and
reinforcement learning bring a full run under $2,000.
