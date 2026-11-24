# Full Preview.2 native OrdWeave integration study

Native Windows source is `b12a007c50c2f9efcbee500a14d3cf09dcc74f10`. Its official dependency is Full v4.1.0 `70d7f0ef5857b54381b863d292a213909d87459f`. This branch changes only research artifacts. Folder names distinguish original archive reproduction, exact-candidate paired comparison, and an independent generated 4096-occurrence holdout. Historical data and source remain in the prior archive at `aa95b240b019c518a98bfb900ec374d24a537165`.

## Reproduce

Use Python 3.12+, native Windows GCC C17, CMake and Ninja. Fetch exact source/archive objects named in the runners. The repositories supplied as arguments are read only. All outputs use new directories.

1. `python -B run_ordweave.py <Full-repo> <OrdWeave-repo> <new-original-output>`. Verifies all 27 trace hashes and complete ZIP membership, and uses the pinned archived driver. Builds exact Stable 4.1 and Preview.1.2 substitution, three formal repeats plus warmup, one separate stage-profile per cell.
2. `python -B final_ordweave.py <Full-repo> <original-output> <new-paired-output>`. Uses all nine seed-17 cases, independently repeats all three exact dependencies, and builds the isolated final profile. Verify Full source `fe00536e7bd7f4e5128432ea4a29e54158074903`.
3. `python -B extra_ordweave.py <OrdWeave-repo> <workdir-containing-capture-and-final> <candidate-diagnostic-static-library> <new-holdout-output>`. The parent must contain `capture` and `final` staging directories from the preceding commands. It builds the slightly enlarged driver and archived upstream Full fault-injection sweep. The source of the fixture is the checked generator in the script.

The application cases below are generated. They exercise typed records, repeated record references, parent collection and logical identity, compare/move/insert/remove, persistent checkpoint, historical revision, reopen and final audit. Temporary directories are kept local. Cases here had a free-disk guard and native process timeout; the independent larger/fault holdout also records memory admission. No field usage, Windows durability guarantees or other-platform support are inferred.

## Analysis

486 original application rows cover 27 traces, memory/persistent modes, two dependencies and independent attribution. Final 234 rows cover nine traces, both modes, three production variants with one warmup and three formal samples, plus a separate attribution process per cell. For each metric use medians of repeats 1–3 per trace. Do not mix instrumented rows with ordinary timing. The profile resets immediately before the operation loop. Rebuild is stage plus publish. WAL commit includes the measured main FlushFileBuffers call, so it must not be added to that call. Run results attribute the timed application blocks only; residual includes API lookup, argument handling, comparator work outside those spans and instrumentation cost.

Final versus Preview.1.2 matched median operation ratios were 0.9963 memory and 0.9892 persistent, with 0.9841 reopen, 1.0057 checkpoint and 1.0001 history. This does not establish meaningful database-wide speedup. The worst checkpoint median ratio was 1.1024 and remains in the archive. In the instrumented run, stage plus publish accounted for a median 94.55% of memory operation time. In persistent mode, WAL commit accounted for a median 97.57% and its main flush 95.12% of operation time. The occurrence rebuild policy and durability boundaries are owned by OrdWeave. No core/adapter change is indicated by the Full wire conversion.

All three dependency builds passed the 12 upstream tests. The larger single-observation persistent holdout passed the sequence, checkpoint, history and reopen oracles on all three controls. The upstream allocator sweep verifies operation-level benign allocation faults, unchanged revision and order, and complete teardown, separately from ordinary performance runs. Additional hardware, workloads and failures remain outside this experiment.
