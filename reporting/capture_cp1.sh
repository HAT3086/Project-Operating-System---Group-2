#!/bin/sh
# Run inside WSL/Linux. Capture actual CP1 evidence and leave a clean tree.
# Usage (from the repository root): sh reporting/capture_cp1.sh
set -eu
LC_ALL=C
export LC_ALL
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../ossim" && pwd)
cd "$project_dir"
out_dir=../reporting/cp1_evidence
mkdir -p "$out_dir"

capture() {
    log_file=$1
    shift
    printf '\n$'
    printf ' %s' "$@"
    printf '\n'
    if "$@" > "$out_dir/$log_file" 2>&1; then
        cat "$out_dir/$log_file"
    else
        command_status=$?
        cat "$out_dir/$log_file"
        printf '\nFAILED (exit %s). See %s/%s\n' "$command_status" "$out_dir" "$log_file"
        exit "$command_status"
    fi
}

{
    uname -a
    gcc --version
    make --version
    git --version
} > "$out_dir/environment.txt" 2>&1

capture clean_before.txt make clean
capture compile.txt make CC=gcc
capture tests.txt make test CC=gcc

for policy in fcfs rr sjf srtf prio mlfq; do
    capture "cp1_hand_$policy.txt" ./ossim workloads/cp1_hand.txt \
        --sched "$policy" --quantum 2 --no-paging --gantt \
        --csv "$out_dir/cp1_hand_$policy.csv"
    capture "arrivals_$policy.txt" ./ossim ../reporting/reference_workloads/arrivals.txt \
        --sched "$policy" --quantum 2 --no-paging --gantt \
        --csv "$out_dir/arrivals_$policy.csv"
    capture "producer_consumer_$policy.txt" ./ossim workloads/producer_consumer.txt \
        --sched "$policy" --quantum 2 --no-paging
done

capture deadlock.txt ./ossim workloads/deadlock.txt --sched rr --quantum 2 \
    --deadlock-every 5 --max-ticks 60 --no-paging --trace
capture multi_instance_safe.txt ./ossim workloads/cp1_resources_safe.txt \
    --sched rr --quantum 1 --deadlock-every 1 --max-ticks 30 --no-paging
capture multi_instance_deadlock.txt ./ossim workloads/cp1_resources_deadlock.txt \
    --sched rr --quantum 1 --deadlock-every 1 --max-ticks 20 --no-paging
capture clean_after.txt make clean

printf '\nCP1 evidence captured in %s/. Source tree cleaned.\n' "$out_dir"
printf 'Test result: '
tail -n 1 "$out_dir/tests.txt"
