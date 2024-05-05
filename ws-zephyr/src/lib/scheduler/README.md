# Scheduler Component

## Intro

Manages execution of threads in the system.

The Scheduler defines OSTask(s) which are executed periodically. An OSTask in turn calls the Runnable(s) which are mapped to them.

### Terms
- OSTask - Priority based preemptible threads which are (usually) scheduled periodically.
- Runnables - The main function of a component.
- Cycle Time - How often an OSTask is triggered.
- Task Mapping - The assignment of which runnables will be executed by a task and the order in which they are executed.

## Code Generation

The majority of scheduler is made of of code generated from the scheduler_map.csv. This ensures that the CSV file always represents the actual scheduler configuration. All generated code is in the [gen/](gen) folder and should never be manually modified.

### Generating code

After updating the [CSV file](./cfg/scheduler_map.csv), run the SchedulerGen.py script:

```
python tools/SchedulerGen.py
```
or if you want something to handle venvs for you, `./tools/gen.sh`

*Note: use --help to show options, inputs or change the output folder.*

### Updating generated code

If non-configuration updates (changes not made to the csv file) are needed. The Jinja2 [templates](tools/templates/) can be found in the tools/templates folder.
