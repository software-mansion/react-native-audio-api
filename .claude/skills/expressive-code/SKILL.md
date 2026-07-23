---
name: expressive-code
description: >
  Guidance for making source code self-documenting and deciding when comments are appropriate.
  Covers naming principles for classes, functions, variables, and constants; cases where syntax
  needs supporting explanation; language-specific documentation styles; and common comment
  anti-patterns. Use when choosing or reviewing names, documenting complex behavior or contracts,
  or adding, editing, reviewing, or removing comments.
  Trigger phrases: "naming principles", "naming conventions", "name this", "rename this",
  "class name", "function name", "variable name", "constant name", "magic constant",
  "self-documenting code", "comment style", "add a comment", "write comments",
  "comment guidelines", "document this", "too many comments", "remove comment", "JSDoc",
  "Doxygen".
---

# Skill: Expressive Code

## Principles

### Self-Documenting Code
Prefer **clean code over comments**. Prefer using **language syntax features** to emphasize semantics, including:
* **Class** and type names describe an object's *traits*.
* **Function** names describe *verbs*.
* **Variable** names describe their *purpose*.
  * In particular, language **constants** explain the *purpose* of values (as opposed to **magic constants**).

#### Names
When possible, a name should **capture semantics fully**.

If the **semantics are complex**, it might be due to one of the following:
* The entity has **too many responsibilities**.
* It is **impossible or infeasible to divide it** further.
  * In this scenario, don't be afraid to use slightly longer names.
  * At the same time, keep in mind that too many long names tend to make code difficult to grasp.
    * Consider adding comments according to the rules.

Generally, prefer standard, idiomatic names.

### Frequency of Comments
Use **comments only when necessary**. Add them only when something cannot be expressed easily in code. Their purpose is to make complex fragments easier to understand. Most code fragments are relatively easy to understand simply by **reading them like prose** (as explained above).

### When Syntax is Not Enough
Some code fragments require additional non-obvious explanations that cannot be expressed through syntax. Common scenarios include:
* Reasons behind **decisions** when other options were available.
  * Including intentional specification deviations.
* Explanations of steps in a **complex algorithm** and/or **abstract** mathematical concepts.
* Documentation of non-obvious **side effects** and/or **implications**.
* **References** to specifications, literature, other relevant sources of knowledge, and credits.
* Complex semantics not easily expressible through short name and/or syntax.
  * Invariants not expressible through assertions, etc.
    * Including assumptions about the environment, timing, threads, and locks.
  * Behavioral contracts.
    * Including **interfaces**, **abstract classes**, and **virtual methods**.

### Comment Style
Use the appropriate documentation style for each programming language. C and C++ typically use Doxygen, while JS and TS use JSDoc.

## Common Anti-Patterns
* Increasing coupling, which makes maintenance harder.
  * Repeating the same or similar comments in many places about the same entity.
    * When adding a comment about a method, for example, place it only in the most important location (such as the header file), so it is easy to find.
  * Restating what can be read explicitly in the code.
  * Taking over Git's responsibilities.
    * Leaving commented-out code.
    * Writing changelog or authorship information.
  * Non-local information.
    * It is dangerous to reference a decision made in a distant location. When the decision changes, it is not obvious that all relevant comments must be updated.
* Information noise.
  * Referencing inaccessible context.
    * Don't record a line of reasoning, as there are many ways to reach a conclusion.
    * Agents tend to reference conversations with their users in code.
  * There is no requirement that *every* function have Javadoc, etc.
  * Imprecision.
    * A general feeling.
      * Example: "The function calculates it well."
    * Reflecting a subjective mental image.
    * Writing a TODO without an actionable next step (and preferably an issue or concrete constraint).
  * A wrong comment is worse than none.

*Maintenance: see [maintenance.md](maintenance.md).*
