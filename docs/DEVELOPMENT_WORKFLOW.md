# NeuroBoost – Development Workflow

> Every development session **must** follow this workflow.
> Skipping any phase causes bugs, regressions, and wasted time.

---

## The Plan-Implement-Test-Document (PITD) Cycle

```
┌─────────┐     ┌───────────┐     ┌────────┐     ┌──────────┐
│  PLAN   │────►│ IMPLEMENT │────►│  TEST  │────►│ DOCUMENT │
└─────────┘     └───────────┘     └────────┘     └──────────┘
     ▲               │                 │
     │               │ Doesn't work    │ Tests fail
     │               ▼                 ▼
     └──────── REVISE PLAN ◄───────────┘
               (don't push through!)
```

**The most important rule:** If your implementation doesn't work or tests fail,
**go back to Phase 1**. Do NOT patch failing code or skip failing tests.
Understanding *why* something fails is more valuable than pushing forward.

---

## Phase 1: PLAN

### Goal

Understand the current state completely before writing any code.

### Steps

1. **Read `docs/STATUS.md`**
   - What is the current phase?
   - What sprint are we on?
   - Are there any blocking issues?
   - Check the component status table.

2. **Read `docs/LESSONS_LEARNED.md`**
   - Read *every* entry, not just recent ones.
   - Identify entries relevant to today's task.
   - Note the prevention rules that apply.

3. **Read relevant source files**
   - Understand what already exists.
   - Do not assume — verify by reading the code.
   - Check for `TODO`, `FIXME`, `HACK` comments.

4. **Write your plan**
   - Describe exactly what you will implement.
   - List every file you will create or modify.
   - List every function you will add or change.
   - Example:
     ```
     PLAN: Implement NoteTracker
     - Create src/dsp/NoteTracker.h
     - Add registerNote(), processBlock(), panicAllNotesOff()
     - Add DSP test in src/NeuroBoostDSP.cpp: verify no hung notes after panic
     - Risks: polyphony overflow → document steal policy
     ```

5. **Identify risks and edge cases**
   - What can go wrong?
   - What are the degenerate inputs?
   - Which realtime safety rules apply?

### Concrete Example

```
PLAN: Add Euclidean rhythm algorithm (Algorithms.h)

Files to create/modify:
- src/dsp/Algorithms.h     (new, add euclidean() function)
- src/dsp/Algorithms.cpp   (new, implement euclidean())
- src/NeuroBoostDSP.cpp    (add test for euclidean edge cases)

Functions to add:
- void euclidean(int k, int n, int rotation, bool* out)

Edge cases to test:
- k=0       → all steps false
- k=n       → all steps true
- k>n       → clamp to n
- n=0       → early return, no writes
- rotation < 0 → safe modulo: ((r % n) + n) % n

Realtime safety check:
- euclidean() uses only stack arrays and integer math → SAFE
- Will be called from ProcessBlock → must be realtime safe

Risks:
- Bjorklund algorithm off-by-one on step distribution → verify with known patterns
  E(3,8) = [1,0,0,1,0,0,1,0] (Tresillo)
  E(4,8) = [1,0,1,0,1,0,1,0]
  E(5,8) = [1,0,1,0,1,0,1,1] (or rotated variant)
```

---

## Phase 2: IMPLEMENT

### Rules

1. **Follow the plan** from Phase 1. Do not add scope mid-implementation.
2. **Check the realtime safety checklist** before adding any line of code
   that will run in `ProcessBlock`.
3. **One logical change at a time.** Complete and verify each change before
   moving to the next.
4. **If something doesn't compile or behave as expected:** STOP.
   Go back to Phase 1, diagnose the root cause, revise the plan.
   Do NOT add workarounds or try random fixes.

### Realtime Safety Micro-Checklist (run before each audio-thread code change)

```
Is this line in the audio thread?
  Yes → check every operation:
    [ ] No new/delete/malloc/free
    [ ] No std::vector growing
    [ ] No std::string construction
    [ ] No mutex/lock/condition_variable
    [ ] No I/O (cout/printf/file)
    [ ] No map/unordered_map inserts
    [ ] No exceptions / throw
    [ ] No std::function (may allocate)
    [ ] No std::random_device / time() / clock()
    [ ] RNG: only mRng (std::mt19937 with user seed)
```

### Code Style

- Match the style of the file you are editing.
- Comments: English only, match existing comment density.
- No commented-out code in commits.
- No `TODO` without a corresponding `STATUS.md` issue.

---

## Phase 3: TEST

### DSP Tests (required for any `src/dsp/` change)

```bash
mkdir -p build && cd build
cmake -DBUILD_DSP_TEST=ON -DBUILD_VST3=OFF -DBUILD_STANDALONE=OFF ..
cmake --build .
ctest --output-on-failure
```

Tests must pass without any Visage or iPlug2 UI headers.

### Determinism Test (required for any algorithm change)

```cpp
// Run the algorithm twice with the same seed, compare output:
auto result1 = runSequencer(seed=42, steps=10);
auto result2 = runSequencer(seed=42, steps=10);
assert(result1 == result2);  // must be identical
```

### Timing Test (required for any MIDI output change)

```cpp
// Verify all sample offsets are valid:
for (auto& event : midiEvents) {
    assert(event.sampleOffset >= 0);
    assert(event.sampleOffset < nFrames);
}
```

### Edge Case Matrix

Every algorithm change must be tested against this matrix:

| Parameter | Min value | Max value | Notes |
|-----------|-----------|-----------|-------|
| `stepCount` | 1 | 64 | Single-step pattern |
| `tempo` | 20 BPM | 300 BPM | Host can send extreme values |
| `nFrames` | 1 | 4096 | Some DAWs use 1-sample blocks |
| `probability` | 0.0 | 1.0 | Always silent / always active |
| `euclidean.hits` | 0 | n | Empty pattern / full pattern |
| `euclidean.rotation` | negative | > n | Safe modulo required |

### If Tests Fail

**Do NOT:**
- Modify tests to pass with broken code.
- Comment out failing assertions.
- Skip the failing test.

**DO:**
- Go back to Phase 1.
- Understand why the test fails.
- Fix the root cause in the implementation.

---

## Phase 4: DOCUMENT

### After every session, you MUST:

1. **Update `docs/STATUS.md`**
   ```
   - Update the Component Status table (mark items in progress or done)
   - Add a Change Log entry with date, description, files modified
   - Update Known Issues (add new issues, mark resolved ones ✅)
   - Update sprint checklist
   ```

2. **Add to `docs/LESSONS_LEARNED.md`**
   ```
   - At least one entry per session
   - Even if nothing went wrong: document what you learned about the codebase
   - Use the standard format: What happened / Root cause / Lesson / Prevention
   ```

3. **Commit with a descriptive message**
   ```
   Good:  "feat(dsp): add NoteTracker with panicAllNotesOff, MAX_POLYPHONY=32"
   Good:  "fix(params): normalize value in SendParameterValueFromUI (was raw 0-200)"
   Bad:   "update code"
   Bad:   "fix bug"
   Bad:   "wip"
   ```

---

## Session Start Protocol

Run this checklist at the start of every session:

```
[ ] Read docs/STATUS.md — understand current phase and sprint
[ ] Read docs/LESSONS_LEARNED.md — note relevant past mistakes
[ ] Identify task from STATUS.md sprint checklist
[ ] Check Known Issues for any blockers
[ ] Create a plan with specific deliverables (Phase 1)
[ ] List risks and edge cases in the plan
```

## Session End Protocol

Run this checklist at the end of every session:

```
[ ] All DSP tests pass (cmake -DBUILD_DSP_TEST=ON && ctest)
[ ] Pre-commit 10-point checklist verified (see AGENTS.md)
[ ] docs/STATUS.md updated (component table, change log, known issues)
[ ] docs/LESSONS_LEARNED.md updated (at least one entry)
[ ] Commit message is descriptive and references the feature/fix
[ ] No temporary debug code (println, commented-out code, TODO without issue)
```

---

## Anti-Patterns to Avoid

| Anti-Pattern | Why It's Harmful | Correct Approach |
|---|---|---|
| "I'll fix the test later" | Broken tests → broken software | Fix root cause now, test must pass |
| "This is just a small change" | Small changes break realtime safety | Always run the micro-checklist |
| "I'll document it next time" | Next agent has no context | Document in Phase 4, every session |
| "The TODO is obvious" | TODOs accumulate and rot | Add to STATUS.md Known Issues |
| "I'll skip the plan for a simple task" | Simple tasks have hidden complexity | Plan first, always |
| "Let me just try this approach" | Thrashing wastes tokens | Plan first, pick one approach |
