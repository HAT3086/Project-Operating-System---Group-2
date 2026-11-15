# ossim: COMP2040 Group 2

Group project for **COMP2040 Operating Systems**. `ossim` is a small kernel
simulator in C. CPU scheduling, semaphores and deadlock, paging and disk
scheduling run together, one tick at a time. We start from the course's
starter code, add the missing policies, then use the simulator for our own
experiments.

## Team

| Member | Checkpoint 1 role |
|---|---|
| Hoang Anh Tu | Schedulers and deadlock detection (implementation), tests |
| Vu Hoang Giang | Scheduler and deadlock-detection design |
| Nguyen Quang Huy | Code review, progress report, submission |

## Status

| Checkpoint | Scope | Status |
|---|---|---|
| 1 (Week 8) | SJF, SRTF, PRIO, MLFQ; deadlock detection; hand-computed tests | Done: `make test` gives 45 passed, 4 skipped, 0 failed |
| 2 (Week 12) | LRU and Clock; SSTF, SCAN and C-LOOK; workload generator; theme choice | Not started (the 4 skipped tests) |
| Final (Week 15) | Theme extension, experiments, 8-10 page report, presentation | Not started |

## Build and test

Needs `gcc` or `clang` and `make` on Linux, macOS or WSL, with no other
libraries. The build uses `-std=c99 -Wall -Wextra -Werror`.

```sh
cd ossim
make
make test
make clean     # run before packaging a submission
```

## Run

```sh
./ossim workloads/basic.txt --sched rr --quantum 4 --no-paging --gantt
./ossim workloads/cp1_hand.txt --sched mlfq --quantum 2 --no-paging --gantt
./ossim workloads/deadlock.txt --sched rr --quantum 2 --deadlock-every 5 --max-ticks 60 --no-paging
```

- `--sched` chooses `fcfs`, `rr`, `sjf`, `srtf`, `prio` or `mlfq`.
- `--trace` prints every state change, `--gantt` the CPU timeline, and
  `--csv out.csv` the per-process metrics.
- `./ossim --help` lists every option.
- The workload format is in `ossim/include/workload.h`.

## Repository layout

| Path | Contents |
|---|---|
| `ossim/` | The simulator: `src/`, `include/`, `tests/`, `workloads/`, `Makefile`. This folder is what gets submitted. |
| `ossim/README.md` | The starter's notes on the code and where each policy goes |
| `reporting/` | Checkpoint 1 reporting guide, captured logs, capture script and report |
| `CP1_HANDOFF.md` | Checkpoint 1 handoff notes |

## Checkpoint 1

- **Report:** `reporting/cp1_report/Group02_CP1.pdf` (LaTeX source alongside).
- **Design notes and hand calculations:** `reporting/CP1_REPORTING_GUIDE.md`.
- **Captured build and test logs:** `reporting/cp1_evidence/`. To regenerate
  them, run `sh reporting/capture_cp1.sh` from the repository root. The script
  ends with `make clean`.
- **Submission:** `Group02_CP1.zip`, containing `Group02_CP1.pdf` and `ossim/`
  after `make clean`, with no `ossim` binary and no `build/`.

## Working rules

- Work on a checkpoint branch and merge into `main` through a reviewed pull request.
- Every member commits their own work. For pair work, the committer adds
  `Co-authored-by: Name <email>` to the commit message.
- Keep hand-computed values and simulator measurements clearly labelled.

## License

MIT. See `LICENSE`.
