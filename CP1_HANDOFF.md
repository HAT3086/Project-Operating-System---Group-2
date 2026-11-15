# Group 2 - Checkpoint 1

The Checkpoint 1 code is complete. Captured WSL verification: **45 passed,
4 expected Checkpoint 2 skips, 0 failed**.

The reporting teammate should start with
[the reporting guide](reporting/CP1_REPORTING_GUIDE.md). It includes the
scheduler and deadlock designs, documented policy choices, independent hand
calculations, measured results, known limitations and the report checklist.
Actual compilation and test output is in
[reporting/cp1_evidence](reporting/cp1_evidence).

## Code and tests

The `ossim/` directory contains only the 42 files needed to build and test
this checkpoint: the Makefile, source, headers, test runner, CP1 case table
and the 16 workload inputs used by the tests. The original memory and disk
policies remain available for the simulator; their Checkpoint 2 stubs are
unchanged.

From WSL/Linux, run:

```sh
cd ossim
make CC=gcc
make test
make clean
```

From the repository root, regenerate the report evidence with:

```sh
sh reporting/capture_cp1.sh
```

This also runs `make clean` at the end. Keep hand calculations and actual
simulator measurements clearly identified when writing the report.

## Submission

When the report is finished, create `Group02_CP1.zip` with two top-level
entries: `Group02_CP1.pdf` (at most two pages) and `ossim/`.

The PDF must include members' names and the repository link at the top,
compilation output, scheduler and deadlock designs, the required metric
comparison, terminal output, limitations and actual member contributions.

Keep the reporting materials in the repository for collaboration. The
final `ossim/` submission must contain no simulator binary or `build/`.
