---
sidebar_position: 2
sidebar_custom_props:
  tags:
    - plan
    - wip
---

# Semi-Automated End-to-End Testing, Debugging and Evidence Collection System (S.A.T.D.E.C.S.)

## Rationale
Some functionalities are only manually tested from time to time. It is because they are not trivially testable automatically (they might require user interaction and/or interaction with the environment). Because of that, the testing process itself is not well defined. Finding regressions is hard and time consuming. Even when a regression is spotted, it is hard to navigate through the evidence, because it first needs to be manually collected from multiple places and filtered. Much of the work can be automated.

## Solution
### Features
The system would provide the following:
* method for manually testing features that aren't trivially testable automatically;
  * Even if there existed an automatic method, it might be unfeasible to implement it from day one.
* uniform framework for creating end-to-end tests;
* logging system with the following features:
  * convenient collection of logs for later analysis,
  * journaling the call trace,
  * Obj C/Kotlin, C++ and JS logs in one timeline,
  * human observations of behavior,
  * robust filtering of all of the above;
* formalized instructions on how to test all functionalities;
* good integration with agents.
  * Agents should be able to write and execute tests in this framework. It would make it easier for them to ask for human feedback in the process of debugging.

### Flow
The key component is a computer interface (e.g. a CLI).
* When a programmer runs a test, it performs all actions that can be automated on a phone (e.g. navigating screens in the Fabric Example).
* Once a human interaction is necessary, an appropriate instruction is displayed and a prompt that allows for providing feedback that will be saved for future analysis. For example it might tell the human to say something, to interrupt the app with another app. Once done, the human journals the outcome (either by picking one of the predefined options or by writing with prose).
* When a test (or a subset of tests) is finished, all relevant logs are available on the development machine.
* It is then possible to filter the logs according to the following criteria:
  * programming language,
  * modules, functions,
  * whether it is a log or a human journal.

### Relation to current solutions
Currently, there are already multiple Fabric Example tests and they would work well with the above generalization. It allows to run all of the tests in a single run with minimal tester interaction. When any of the tests fails, it would allow to easily access, filter and navigate the timeline of tests executions and outcomes from a computer. It is convenient to control the testing process from a computer, but it doesn't need to be necessary.

### Logs structure
Each run should be stored in a folder named by the commit hash, optionally name of what it tests and a timestamp. The folder would store logs from all programming languages and also the journal. It is not necessary to store all logs in a single file as it would get too messy. One thing is to keep in mind that all logs have timestamps. It allows for later combining it into one timeline. The tool that would display the combined timeline should also have options to trigger the filters. It also might be beneficial to filter logs before saving them to the drive, so they don't take too much space.

## Implementation order
### Basic features
The initial implementation should only consist of what is most useful in the short term:

* configurable logging system on iOS in the Objective C codebase;
  * logs code only compiled on debug target,
  * ability to trigger trace logs on/off on runtime (via an environment variable for example),
* standardized collection of logs;
* minimal tool that records logs and allows the tester to journal their experience with respect to chronology;
* instructions in internaldocs on how to test some functionalities that require human interaction.

### Maximizing automation
Above implementation makes testing and debugging significantly easier. Yet it still leaves place for automating multiple steps of a test and doesn't allow to formalize them in a test language. Because of that, agents have less possibilities of debugging.

There are a few possible solutions. The main problem is that most end-to-end mobile testing frameworks don't have a step that allows for a manual interaction.

#### Argent
The tests could be written in Argent via YAML flow files. Argent has a directive for executing local scripts. The script that would be run would display a prompt for the tester that would allow to collect the feedback in prose:

```yaml
- echo: Interrupt playback with a phone call, then say whether audio resumed.
- script: { path: ../../scripts/ask.mjs, timeout: 180000 }
```

There are two possible problems with this approach: According to [Argent docs](https://docs.swmansion.com/argent/docs/reference/flow-yaml/#local-scripts), local scripts are meant for "setup or cleanup that device steps cannot do", so this solution serves as a workaround. Another problem is that the `script` directive doesn't allow to pass any arguments to the script, therefore the following **would not** work:

```yaml
 - script:
     path: ../../scripts/ask.mjs
     args: ["Interrupt playback with a phone call", "Outcome A", "Outcome B"]
```

Without arguments, the prompt dialog would not display the instruction for the tester and would not contain predefined answers. That is only an inconvenience when the tester is reading the terminal where the `echo` is printed. During an agent run the tester may see only the dialog, so the instruction never reaches them. The dialog also cannot be an `stdin` prompt in that terminal, because the tool-server starts the script as a child process, and that process does not read the terminal's keyboard.

There are two possible cases to adjust Argent codebase for the usecase of live interaction with a tester:
* The simplest solution is to add support for commandline arguments to the scripts.
* Another is to straightly ask the Argent team to add support for a new `prompt` directive. It would allow to add an option of human interaction and feedback, when it is impossible to automize something. It could have the following syntax:

  ```yaml
  - prompt:
      instruction: "Say something and tell whether the recording can be heard clearly."
      outcomes: ["Outcome A", "Outcome B"]
  ```

#### Appium
One of the alternative testing frameworks is [Appium](https://github.com/appium/appium). It's got the following features:
* It supports physical iPhones and Android devices.
* The tests can be written in a programming language (including JS and Java), which allows for extending them with custom instructions such as a prompt for a human to do something and provide feedback.
* It can be controlled by agents through [`appium-mcp`](https://github.com/appium/appium-mcp).
