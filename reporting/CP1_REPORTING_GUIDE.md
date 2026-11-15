# Group 2 - Checkpoint 1 reporting guide

Prepared: 15 November 2026. This is a handoff for the teammate writing the
progress report. It is supporting material, not the final two-page PDF.

Repository: https://github.com/HAT3086/Project-Operating-System---Group-2

## Repository layout for the reporting teammate

- `ossim/`: the minimal code submission (42 files).
- `reporting/`: this guide, terminal evidence and the capture utility.
- `reporting/reference_workloads/`: optional starter inputs, outside the submission folder.

Code paths in this guide are relative to `ossim/`; evidence paths are
relative to `reporting/`. Only `ossim/` and your finished report PDF belong
in the final submission ZIP.

## 1. Verified status and scope

The Checkpoint 1 implementations are complete and the captured WSL build
and tests succeeded:

```text
45 passed, 4 skipped (not implemented yet), 0 failed
```

This is the actual last line of `cp1_evidence/tests.txt`. The four skips are
the starter's Checkpoint 2 LRU, SSTF, SCAN and C-LOOK tests. They are expected
at this checkpoint. All 36 new Checkpoint 1 checks passed, along with the
9 starter reference checks. Producer/consumer completed under all six
schedulers, including FCFS and RR.

Only three original files were edited:

| File | Change |
|---|---|
| `src/sched.c` | Implement SJF, SRTF, PRIO and MLFQ using the existing scheduler hooks. |
| `src/sync.c` | Implement resource deadlock detection. |
| `tests/run.sh` | Load CP1 cases; check command exit status and literal expected output; add absence and single-occurrence checks. |

New files are the `workloads/cp1_*.txt` test inputs,
`tests/cp1_cases.tsv`, plus the guide, capture utility and evidence
kept separately in `reporting/`. The original kernel, queue implementation, process model,
memory policies, disk policies, headers, Makefile, README and supplied
workloads were not edited. FCFS and RR scheduling behavior is unchanged.

No workload generator or theme extension was added. The final capture script
ran `make clean`: the code folder contains neither `build/` nor an `ossim`
binary.

## 2. What the schedulers do

All ready queues use the supplied `queue_t`, an array of PCB pointers.
`q_push()` appends to the tail. `q_pop_min()` takes the earliest queued
process with the smallest key; it preserves FIFO order on equal keys.
Consequently FIFO means order of entry to the ready queue, not lowest PID.

| Policy | Queues and selection | Preemption and tie rule |
|---|---|---|
| SJF | One ready queue; `q_pop_min()` with `proc_burst_remaining`. The key is the current CPU burst, not all future CPU work. | Non-preemptive. Equal burst lengths use ready-queue FIFO order. |
| SRTF | One ready queue; the same minimum remaining-burst selection. | Preempt before a CPU tick only when a ready process has **strictly less** remaining burst time than the running process. Equality keeps the CPU; selection ties use FIFO. |
| PRIO | One ready queue; stable minimum of `p->priority`. Smaller numbers have higher priority. | A higher-priority ready process preempts immediately. Equal priorities rotate after `q` CPU ticks. Lower priorities do not force rotation. Requeued processes join the tail; each dispatch starts a fresh quantum. |
| MLFQ | Three FIFO queues, levels 0, 1 and 2. Always select the head of the highest nonempty queue. Quanta are `q`, `2q`, `4q`. | Higher-level ready jobs preempt lower-level running jobs. Equal-level jobs rotate when the running job exhausts its quantum. A full quantum demotes one level, capped at level 2. An early block keeps the level. Periodic boosts reset all jobs to level 0. |

The simulator knows exact burst lengths for SJF/SRTF. A real OS could
estimate the next burst with exponential averaging:
`tau_next = alpha * actual_burst + (1 - alpha) * tau_previous`.
Burst prediction is an explanation, not a feature implemented for CP1.

### MLFQ choices to state explicitly in the report

The handout specifies levels, budgets, demotion and a 20-tick boost, but
does not fully specify the following details. These are our choices:

1. New processes enter level 0. All ready-queue insertions are at the tail,
   including a process interrupted by a higher-level arrival.
2. An interruption by a higher-level ready job preserves the CPU ticks
   already used in the current quantum. An early blocking operation gives
   a fresh quantum at the same level when the process becomes ready again.
3. Quantum exhaustion is handled immediately after the last CPU tick of
   that quantum. A process that blocks **exactly at** the boundary is
   therefore demoted; one that blocks earlier is not.
4. A boost occurs at the start of ticks 20, 40, 60, ... after that tick's
   arrival/wakeup events and before the scheduling decision. Ready queues
   merge in old level order (Q0, then Q1, then Q2), retaining FIFO within
   each. The running process joins the tail if another process is ready.
5. Boosts reset levels and quantum counters for blocked processes too.
   The scheduler checks for boosts even while the CPU is idle. A lone
   running job continues with its reset budget.
6. Consecutive CPU operations separated only by zero-time operations keep
   the CPU as the original kernel specifies. Their CPU ticks still count
   toward the current PRIO/MLFQ quantum.

For PRIO, a higher-priority interruption is followed by a fresh quantum on
redispatch. This is also a documented choice where the handout is brief.

## 3. How deadlock detection works

`deadlock_check()` uses the textbook multi-instance resource detection
algorithm (Section 8.6.2), not Banker's avoidance algorithm.

1. Copy each resource's `available` count to a local `Work` vector.
2. For each PID, initially mark `Finish=true` if it holds no resources;
   otherwise set `Finish=false`.
3. Repeatedly find an unfinished process whose current `requested` vector
   is no greater than `Work` in every resource type. Hypothetically allow
   it to finish, add its `allocated` vector to `Work`, and mark it finished.
4. When no further process can be marked, the remaining unmarked resource
   holders are the detected participants. Print their PIDs in increasing
   order. Suppress repeated output while this set is unchanged.

The detector does not alter the resource state or kill a victim. On the
supplied deadlock input with RR, `q=2`, checks every five ticks, paging off,
the actual output is:

```text
DEADLOCK t=15 pids: 1 2
```

A holds R1 and requests R2; B holds R2 and requests R1. No request can be
satisfied from the available resources. Independent C still finishes:

```text
ticks=60 finished=1/3 aborted=0
```

The warning that A and B remain unfinished is expected when detection is
enabled without recovery. The persistent set is printed only once.

Tests also cover safe temporary waiting with two instances of one resource,
and a true multi-instance deadlock where A and B each hold one of three
instances and each request two more (`Available=1`). A waiter holding
nothing is excluded from the textbook detector's participant set.

## 4. Independent hand calculation for the report

Use `workloads/cp1_hand.txt` as the report's primary comparison. The
assignment expressly permits a workload of our own. Its small size makes
the hand calculation auditable within the two-page limit. All runs use
`--quantum 2 --no-paging`.

| Process | Arrival | CPU burst | Priority |
|---|---:|---:|---:|
| P1 | 0 | 6 | 2 |
| P2 | 1 | 3 | 1 |
| P3 | 2 | 1 | 1 |

### Hand-written schedules

The intervals below are **half-open**: `0-6 P1` means P1 executes at ticks
0 through 5 and finishes at time 6. The simulator's printed `[0-5] P1`
means the same thing: its timeline endpoints are inclusive tick labels.

| Scheduler | Hand-written CPU intervals |
|---|---|
| FCFS | 0-6 P1; 6-9 P2; 9-10 P3 |
| RR | 0-2 P1; 2-4 P2; 4-5 P3; 5-7 P1; 7-8 P2; 8-10 P1 |
| SJF | 0-6 P1; 6-7 P3; 7-10 P2 |
| SRTF | 0-1 P1; 1-2 P2; 2-3 P3; 3-5 P2; 5-10 P1 |
| PRIO | 0-1 P1; 1-3 P2; 3-4 P3; 4-5 P2; 5-10 P1 |
| MLFQ | 0-2 P1; 2-4 P2; 4-5 P3; 5-9 P1; 9-10 P2 |

Why these schedules follow the rules:

- FCFS runs in arrival order.
- In RR, P3 arrives at tick 2 before the expired P1 is requeued, so P3
  precedes P1. The ready order at that point is P2, P3, P1.
- SJF cannot interrupt P1. When P1 finishes, P3's burst of 1 beats P2's 3.
- In SRTF, P2 interrupts P1 at tick 1 (`3 < 5`); P3 interrupts P2 at
  tick 2 (`1 < 2`). P2 then finishes before P1.
- In PRIO, P2 interrupts P1 at tick 1. P3 has the same priority as P2,
  so it waits until P2's two-tick quantum ends at time 3. Then P3 runs,
  P2 finishes, and P1 resumes.
- In MLFQ, P1 and P2 each consume two ticks and move to Q1. P3 runs in
  Q0, then Q1 selects P1 before P2. P1's next four ticks finish it.
  This small case ends before the first boost; dedicated tests check
  boosts, early blocks, exact-boundary blocks and interrupted budgets.

### Per-process arithmetic

Let A = arrival, S = first dispatch, F = finish and C = total CPU time.
For this CPU-only workload:

```text
Turnaround T = F - A
Response   R = S - A
Waiting    W = T - C
Average      = sum of the three process values / 3
```

For a workload with I/O or other blocking, waiting is the time spent READY;
do not use `T - C` alone. Blocking time must also be excluded.

| Scheduler | PID | S | F | W | R | T |
|---|---|---:|---:|---:|---:|---:|
| FCFS | P1 | 0 | 6 | 0 | 0 | 6 |
| FCFS | P2 | 6 | 9 | 5 | 5 | 8 |
| FCFS | P3 | 9 | 10 | 7 | 7 | 8 |
| RR | P1 | 0 | 10 | 4 | 0 | 10 |
| RR | P2 | 2 | 8 | 4 | 1 | 7 |
| RR | P3 | 4 | 5 | 2 | 2 | 3 |
| SJF | P1 | 0 | 6 | 0 | 0 | 6 |
| SJF | P2 | 7 | 10 | 6 | 6 | 9 |
| SJF | P3 | 6 | 7 | 4 | 4 | 5 |
| SRTF | P1 | 0 | 10 | 4 | 0 | 10 |
| SRTF | P2 | 1 | 5 | 1 | 0 | 4 |
| SRTF | P3 | 2 | 3 | 0 | 0 | 1 |
| PRIO | P1 | 0 | 10 | 4 | 0 | 10 |
| PRIO | P2 | 1 | 5 | 1 | 0 | 4 |
| PRIO | P3 | 3 | 4 | 1 | 1 | 2 |
| MLFQ | P1 | 0 | 9 | 3 | 0 | 9 |
| MLFQ | P2 | 2 | 10 | 6 | 1 | 9 |
| MLFQ | P3 | 4 | 5 | 2 | 2 | 3 |

Example: under PRIO, P1 has `T=10-0=10`, `R=0-0=0`, and `W=10-6=4`.
The PRIO average waiting time is `(4+1+1)/3 = 2.00` ticks.

### Table to use in the two-page report

These are independently calculated values next to actual captured WSL
simulator results, rounded to two decimal places. Every value agrees.

| Scheduler | Hand W | ossim W | Hand R | ossim R | Hand T | ossim T |
|---|---:|---:|---:|---:|---:|---:|
| FCFS | 4.00 | 4.00 | 4.00 | 4.00 | 7.33 | 7.33 |
| RR | 3.33 | 3.33 | 1.00 | 1.00 | 6.67 | 6.67 |
| SJF | 3.33 | 3.33 | 3.33 | 3.33 | 6.67 | 6.67 |
| SRTF | 1.67 | 1.67 | 0.00 | 0.00 | 5.00 | 5.00 |
| PRIO | 2.00 | 2.00 | 0.33 | 0.33 | 5.33 | 5.33 |
| MLFQ | 3.67 | 3.67 | 1.00 | 1.00 | 7.00 | 7.00 |

The raw independent arithmetic is in `cp1_evidence/hand_calculations.csv`;
the actual per-process simulator results are in
`cp1_evidence/cp1_hand_<scheduler>.csv`. All 18 rows agree on arrival,
first dispatch, finish, waiting, response and turnaround (108 comparisons).

### Optional supplied arrivals workload

The capture script also ran the original `workloads/arrivals.txt` with
`q=2`, paging off. Actual simulator averages are:

| Scheduler | Waiting | Response | Turnaround |
|---|---:|---:|---:|
| FCFS | 14.83 | 10.17 | 22.67 |
| RR | 18.33 | 3.17 | 26.17 |
| SJF | 10.00 | 9.00 | 17.83 |
| SRTF | 9.83 | 9.00 | 17.67 |
| PRIO | 12.83 | 8.33 | 20.67 |
| MLFQ | 16.33 | 0.00 | 24.17 |

These additional results are simulator measurements, not claimed as a
separate hand calculation. Use the small hand case above for the required
comparison. The original arrivals input and its I/O timing are unchanged.

## 5. Terminal evidence and reproduction

The successful captured environment is WSL2 Linux, GCC 15.2.0,
GNU Make 4.4.1 and Git 2.53.0. Full details are in
`cp1_evidence/environment.txt`.

From the WSL terminal, reproduce everything with:

```bash
cd "/mnt/d/COMP2040FAL26 - Operating System/Project"
sh reporting/capture_cp1.sh
```

The script builds with the original Makefile and C99 warning flags, runs
`make test`, captures all six policies on both workloads, records
producer/consumer and deadlock results, then cleans the build. It stops
on a failing command, displaying its saved output.

| Evidence file | What to use it for |
|---|---|
| `compile.txt` | Actual complete `make CC=gcc` output. Include a legible excerpt in the PDF. |
| `tests.txt` | Every PASS/SKIP and the final test summary. |
| `cp1_hand_<policy>.txt` | Actual small-case per-process table, inclusive-tick Gantt timeline and averages. |
| `cp1_hand_<policy>.csv` | Actual small-case process metrics, convenient for cross-checking the table. |
| `hand_calculations.csv` | Arithmetic from the independent hand-written schedules. |
| `arrivals_<policy>.txt` and `.csv` | Extra supplied-workload measurements. |
| `producer_consumer_<policy>.txt` | Proof of `finished=2/2` for every scheduler. |
| `deadlock.txt` | Full trace, one sorted deadlock line, C's completion, expected unfinished-job warning. |
| `multi_instance_safe.txt` | No false resource-deadlock report; both jobs finish. |
| `multi_instance_deadlock.txt` | Multi-instance resource detection and exclusion of a zero-allocation waiter. |
| `clean_after.txt` | Actual final `make clean` output. |

All the above filenames are under `cp1_evidence/`. The separate
`native_compile.txt` is only an earlier Windows compilation diagnostic;
use the WSL logs for report evidence.

A compact actual output excerpt for the report is:

```text
gcc -std=c99 -Wall -Wextra -Werror -O2 -g -Iinclude -c -o build/sched.o src/sched.c
gcc -std=c99 -Wall -Wextra -Werror -O2 -g -Iinclude -c -o build/sync.o src/sync.c
45 passed, 4 skipped (not implemented yet), 0 failed
DEADLOCK t=15 pids: 1 2
```

Label this as an excerpt from separate compile, test and deadlock runs.
The full compile log also contains the successful linker command. Add
that line or a screenshot of the log if there is room. Do not present an
invented terminal screenshot or an expected result as actual execution.

## 6. Limitations and rules to discuss

- No failing behavior was observed in the CP1 checks. State that this is
  the extent of the validation; it is not a proof for every workload.
- The MLFQ boost order, treatment of blocked jobs and partial-quantum
  rules are implementation choices described above. They should be
  highlighted as interpretations that the checkpoint can give feedback on.
- Priority aging is not implemented. Continuous high-priority work can
  starve a low-priority job; aging is a stretch goal in the starter.
- Deadlock recovery is not implemented; detection-only reporting is
  expressly allowed. The intentionally deadlocked jobs stay blocked until
  `--max-ticks` stops the run.
- Detection covers named resource Allocation/Request dependencies, not
  semaphore-only cycles. Like the textbook algorithm, it assumes a job
  whose current request fits can eventually finish and release what it
  holds. It does not analyze future operations, semaphore dependencies or
  extra FIFO head-of-line dependencies in resource waiting queues.
- LRU, Clock, SSTF, SCAN, C-LOOK, the workload generator and theme work
  remain for later checkpoints. No changes were made to those parts.

## 7. Suggested two-page layout

**Top of page 1:** Group 2, all three real names (and student IDs if your
course requires them), and the repository URL. Names have not yet been
provided; fill them in before submission.

**Page 1:** A concise four-row scheduler design table, the chosen MLFQ
rules, and a short explanation of multi-instance deadlock detection.

**Page 2:** State the small workload and `q=2, --no-paging`; include the
hand-versus-ossim averages table and one short arithmetic example. Include
compile/test/deadlock output excerpts, limitations, and three contribution
lines. The detailed 18-row working above is supporting material; there is
no need to fit it all into the two-page PDF.

Record actual contributions. Based on the current arrangement:

| Member | Contribution line to complete honestly |
|---|---|
| [Your name] | Led CP1 coding and testing with AI assistance and ran WSL verification; specify the parts you personally implemented or reviewed and can explain. |
| [Reporting teammate's name] | Prepared the progress report, checked its figures against the logs, and organized submission (record these as completed only after doing them). |
| [Other coding teammate's name] | Currently unavailable for CP1 coding; describe any actual review/testing completed before submission. If none, state no CP1 coding contribution. |

The course requires the team to understand every submitted line and use
one group repository with real member contributions. Use your group's real repository and your own identity for commits,
preserving any existing work. The repository should record the actual
contributions of each member. Do not
attribute code to a teammate who did not write or review it.

## 8. Submission steps

1. Finish the report and export it as **`Group02_CP1.pdf`**, at most two
   pages. Include the member names, repository link and compilation output.
2. Make sure any final source changes have been retested. Run `make clean`
   inside `ossim/` after testing. The captured version is already clean.
3. Assemble **`Group02_CP1.zip`** with these top-level entries:
   - `Group02_CP1.pdf`
   - `ossim/` containing source, headers, Makefile, tests and workloads.
4. Check the archive excludes the simulator binary, `build/`, `.git/`,
   macOS metadata (`.DS_Store`, `__MACOSX`, `._*`) and development scratch
   files. The guide and evidence stay in `reporting/`, outside the submitted `ossim/` folder.
5. One member uploads the final ZIP. This handoff has no final PDF because
   the reporting teammate is responsible for writing it.

If the instructor uses `Group2_CP1` without a leading zero, use that
spelling consistently for the PDF and ZIP. `Group02` follows the supplied
`GroupXX` naming pattern.
