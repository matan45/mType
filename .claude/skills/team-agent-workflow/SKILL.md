---
name: team-agent-workflow
description: Multi-agent software delivery workflow for mType tasks using Testing, Architect, Coder, and Reviewer roles. Use when the user asks for a team agent, parallel sub-agents, collaborative implementation planning, design review, test strategy, code review, or coordinated feature/bug-fix execution across mType components.
---

# Team Agent Workflow

Use this skill to coordinate substantial mType work through four specialist roles:

- Testing: define behavior, regression risk, fixture strategy, and verification commands.
- Architect: evaluate design shape, module boundaries, coupling, and long-term maintainability.
- Coder: propose the implementation path with concrete files, APIs, and sequencing.
- Reviewer: challenge the plan or diff for bugs, missing tests, regressions, and scope creep.

For detailed role prompts, decision gates, and merge rules, read [REFERENCE.md](REFERENCE.md).

## Operating Rules

Start with repository context before launching role work. Inspect the relevant files, existing tests, and nearby patterns.

Use parallel sub-agents when the runtime supports them. If sub-agents are unavailable, run the four roles sequentially in separate clearly labeled passes.

Do not let role outputs replace judgment. Merge them into one concrete plan, resolve conflicts explicitly, and prefer the smallest behavior-preserving implementation that satisfies the user request.

For read-only brainstorm tasks, stop after the merged recommendation. For implementation tasks, proceed after the merged plan unless the user explicitly asked for planning only.

## Workflow

1. Classify the task:
   - Bug fix: prioritize Testing and Reviewer.
   - New language feature: prioritize Architect, Testing, and Coder.
   - Refactor: prioritize Architect and Reviewer.
   - Tooling/UI work: include Coder and Reviewer with focused test coverage.

2. Gather context:
   - Read the relevant source files.
   - Find existing tests or fixtures.
   - Check local skill guidance that applies, such as `tdd`, `refactor`, or `mtype-language-feature-tests`.

3. Launch role passes:
   - Testing produces behaviors, risks, and verification strategy.
   - Architect produces design constraints and module-boundary guidance.
   - Coder produces implementation sequence and file touch list.
   - Reviewer produces likely failure modes and review gates.

4. Merge:
   - Identify agreements.
   - Resolve disagreements.
   - Drop speculative work.
   - Produce one implementation plan or read-only recommendation.

5. Execute:
   - For code changes, edit in small steps.
   - Verify with the narrowest useful tests.
   - Report unresolved risks honestly.

## Conflict Resolution

Testing wins on observable behavior and regression coverage.

Architect wins on module boundaries and long-term coupling.

Coder wins on local implementation feasibility after reading the code.

Reviewer wins on demonstrated correctness risks.

When two roles disagree, prefer the option that is easier to test and easier to revert.
