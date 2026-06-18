# Team Agent Workflow Reference

Use this reference after loading [SKILL.md](SKILL.md). Keep role work independent until the merge step so disagreement remains visible.

## Role Briefs

### Testing

Mission: define the behavioral contract and verification strategy.

Inputs:
- User request
- Relevant source files
- Existing test suite patterns
- Known mType testing conventions

Output:
- Behaviors to preserve or add
- Regression risks
- Existing tests to run
- New tests or fixtures needed
- Cases that should not be tested because they are implementation details

Bias:
- Prefer observable behavior.
- Prefer existing test harnesses.
- Avoid tests coupled to private implementation details.

### Architect

Mission: protect design quality and module boundaries.

Output:
- Affected bounded context
- Existing abstractions to reuse
- Abstractions to avoid adding
- Coupling or ownership risks
- Recommended shape of the change

Bias:
- Prefer existing mType architecture.
- Keep modules deep and interfaces small.
- Avoid broad rewrites during feature or bug work.

### Coder

Mission: turn the request into an implementation sequence.

Output:
- Files likely touched
- Step-by-step edit plan
- Important call sites
- Build or test considerations
- Known unknowns requiring inspection

Bias:
- Read before editing.
- Make minimal coherent changes.
- Follow local style and naming.

### Reviewer

Mission: find bugs before they land.

Output:
- Correctness risks
- Edge cases
- Missing tests
- Overreach or unrelated refactors
- Final review gates

Bias:
- Findings first.
- Use concrete file/function references when available.
- Avoid style noise unless it affects maintainability or correctness.

## Parallel Prompt Templates

### Testing Prompt

You are the Testing role for this mType task.

Analyze the request and relevant code context. Produce:
1. Observable behaviors that must be preserved or added.
2. Regression risks.
3. Existing tests or fixtures likely relevant.
4. New tests needed, if any.
5. Verification commands or manual validation needed.

Do not propose implementation details unless they affect testability.

### Architect Prompt

You are the Architect role for this mType task.

Analyze the request and relevant code context. Produce:
1. The owning module or bounded context.
2. Existing patterns or abstractions to follow.
3. Design risks.
4. Abstractions to add, avoid, or reuse.
5. Recommended shape of the change.

Prefer the smallest design that keeps the codebase coherent.

### Coder Prompt

You are the Coder role for this mType task.

Analyze the request and relevant code context. Produce:
1. Likely files to inspect or edit.
2. Concrete implementation sequence.
3. Call sites and registrations to update.
4. Build or test considerations.
5. Any blockers or assumptions.

Keep the plan scoped and executable.

### Reviewer Prompt

You are the Reviewer role for this mType task.

Analyze the request, proposed approach, and relevant code context. Produce:
1. Likely bugs or regressions.
2. Missing tests.
3. Edge cases.
4. Scope creep.
5. Review gates before completion.

Prioritize correctness and behavioral risk.

## Workflow Gates

### Scope Gate

Confirm whether the task is language, runtime, tooling, docs, test-only, or cross-cutting. Name the affected surfaces before planning implementation.

### Semantics Gate

For language and runtime work, define intended behavior precisely enough to test. Include invalid programs and expected diagnostic shape when relevant.

### Test Gate

Testing produces the minimum useful matrix before Coder starts:
- Basic valid behavior
- Representative invalid behavior, when applicable
- Regression coverage for bugs
- Edge cases around scope, type, runtime, or tooling behavior

### Implementation Gate

Coder proceeds only after the design and test constraints are clear. Keep edits vertical and scoped; avoid unrelated cleanup.

### Review Gate

Reviewer checks that behavior matches the merged plan, tests cover the important risks, and implementation follows existing mType architecture.

### Acceptance Gate

Run targeted tests first, then broaden according to blast radius:
- Parser/type tests for syntax or type changes
- VM/runtime tests for bytecode or execution changes
- Language-server tests for editor/tooling changes
- Benchmark or perf checks for hot paths

## Merge Template

Use this after collecting role outputs:

```text
Agreements:
- ...

Conflicts:
- ...

Decision:
- ...

Implementation Plan:
1. ...

Verification:
- ...

Deferred:
- ...
```

## Conflict Resolution Details

Preserve role disagreement until the merge step. Do not simulate consensus.

When Testing and Coder disagree, prefer the behavior that can be verified through a stable public interface.

When Architect and Coder disagree, prefer the implementation that preserves locality and follows existing mType patterns unless the code proves it is infeasible.

When Reviewer flags a correctness risk, either address it or list it as an explicit residual risk.

When all options are viable, choose the one that is easiest to test and easiest to revert.

## Fallback Mode

If parallel sub-agents are unavailable, run the same roles sequentially in isolated sections:

1. Testing pass
2. Architect pass
3. Coder pass
4. Reviewer pass
5. Merge

Do not revise earlier role sections while writing later sections. Reconcile them only in the merge.
