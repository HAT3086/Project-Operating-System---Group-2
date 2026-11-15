#!/bin/sh
# Tests against hand-computed textbook numbers. Extend this file as you
# implement policies (e.g. SJF/SRTF/PRIO/MLFQ on arrivals.txt, your own
# paging and disk workloads).
#
# A check whose command stops with "not implemented yet" prints SKIP: the
# expected value is already here, waiting for your implementation. Every SKIP
# must have become PASS by the deadline of its checkpoint.
BIN=${BIN:-./ossim}
fail=0; pass=0; skip=0

check() {   # check <description> <command> <expected-word>
    status=0
    out=$($2 2>&1) || status=$?
    if echo "$out" | grep -q "not implemented yet"; then
        echo "SKIP  $1"; skip=$((skip + 1))
    elif [ "$status" -eq 0 ] && printf '%s\n' "$out" | grep -Fq -- "$3"; then
        echo "PASS  $1"; pass=$((pass + 1))
    else
        echo "FAIL  $1"; echo "      expected: $3"; echo "$out" | sed 's/^/      /'; fail=1
    fi
}

check_absent() {   # command must succeed and must not contain the token
    status=0
    out=$($2 2>&1) || status=$?
    if [ "$status" -eq 0 ] && ! printf '%s\n' "$out" | grep -Fq -- "$3"; then
        echo "PASS  $1"; pass=$((pass + 1))
    else
        echo "FAIL  $1"; echo "$out"; fail=1
    fi
}

check_once() {     # command must succeed and print the token on one line only
    status=0
    out=$($2 2>&1) || status=$?
    count=$(printf '%s\n' "$out" | grep -Fc -- "$3")
    if [ "$status" -eq 0 ] && [ "$count" -eq 1 ]; then
        echo "PASS  $1"; pass=$((pass + 1))
    else
        echo "FAIL  $1"; echo "      expected one line containing: $3"; echo "$out"; fail=1
    fi
}

# --- reference policies (must pass from day one) --------------------------
check "FCFS textbook example: avg waiting 17.00" \
      "$BIN workloads/basic.txt --sched fcfs --no-paging" "avg_waiting=17.00"

check "RR q=4 textbook example: avg waiting 5.67" \
      "$BIN workloads/basic.txt --sched rr --quantum 4 --no-paging" "avg_waiting=5.67"

check "RR q=4: context switches = 4 (P1,P2,P3,P1)" \
      "$BIN workloads/basic.txt --sched rr --quantum 4 --no-paging" "context_switches=4"

check "Paging: P1 alone, FIFO, 3 frames -> 15 faults (textbook Sec. 10.4.2)" \
      "$BIN workloads/paging_p1.txt --frames 3 --replace fifo" "page_faults=15"

check "Disk: FCFS, head 53 -> 640 cylinders (textbook Sec. 11.2.1)" \
      "$BIN workloads/disk_textbook.txt --no-paging --head 53 --disk fcfs" "head_movement=640"

check "Producer/consumer completes" \
      "$BIN workloads/producer_consumer.txt --sched rr --quantum 2 --no-paging" "finished=2/2"

check "Deadlock workload without detection: A and B stall, C finishes" \
      "$BIN workloads/deadlock.txt --sched rr --quantum 2 --max-ticks 60 --no-paging" "finished=1/3"

check "Mixed workload finishes" \
      "$BIN workloads/mixed.txt --sched rr --quantum 3 --frames 6" "finished=4/4"

check "Mixed workload finishes with 1 frame (faults wait for a frame)" \
      "$BIN workloads/mixed.txt --sched rr --quantum 3 --frames 1" "finished=4/4"

# --- checkpoint 1 ----------------------------------------------------------
cp1_skip_before=$skip
# Expected values are independent hand calculations. The tab-separated
# table keeps commands and expected results easy to inspect and reproduce.
while IFS="$(printf '\t')" read -r kind description arguments expected; do
    case "$kind" in
        contains) check "$description" "$BIN $arguments" "$expected" ;;
        absent)   check_absent "$description" "$BIN $arguments" "$expected" ;;
        once)     check_once "$description" "$BIN $arguments" "$expected" ;;
        ''|'#'*)  continue ;;
        *) echo "FAIL  unknown CP1 test kind: $kind"; fail=1 ;;
    esac
done < tests/cp1_cases.tsv
if [ "$skip" -ne "$cp1_skip_before" ]; then
    echo "FAIL  Checkpoint 1 policies must not be skipped"; fail=1
fi

# --- checkpoint 2 ----------------------------------------------------------
check "Paging: LRU, 3 frames -> 12 faults (textbook Sec. 10.4.4)" \
      "$BIN workloads/paging_p1.txt --frames 3 --replace lru" "page_faults=12"
# TODO: a Clock case you computed by hand (the textbook has none, make one).

check "Disk: SSTF, head 53 -> 236" \
      "$BIN workloads/disk_textbook.txt --no-paging --head 53 --disk sstf" "head_movement=236"
check "Disk: SCAN moving down, head 53 -> 236" \
      "$BIN workloads/disk_textbook.txt --no-paging --head 53 --direction down --disk scan" "head_movement=236"
check "Disk: C-LOOK moving up, head 53 -> 322" \
      "$BIN workloads/disk_textbook.txt --no-paging --head 53 --direction up --disk clook" "head_movement=322"

echo "---"
echo "$pass passed, $skip skipped (not implemented yet), $([ $fail = 0 ] && echo 0 || echo some) failed"
exit $fail
