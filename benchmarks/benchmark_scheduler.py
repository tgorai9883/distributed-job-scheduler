#!/usr/bin/env python3

import csv
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "build"
APP = BUILD_DIR / "scheduler_app"
RESULTS = ROOT / "benchmarks" / "results.csv"

SCENARIOS = [
    ("independent_100", "independent", 100),
    ("independent_1000", "independent", 1000),
    ("chain_100", "chain", 100),
    ("diamond_batch", "diamond_batch", 202),
]

WORKER_COUNTS = [1, 2, 4, 8]


def run(command):
    return subprocess.run(
        command,
        cwd=ROOT,
        check=True,
        text=True,
        capture_output=True,
    )


def build():
    run(["cmake", "-S", ".", "-B", str(BUILD_DIR)])
    run(["cmake", "--build", str(BUILD_DIR), "--target", "scheduler_app"])


def run_benchmark(scenario_name, app_scenario, task_count, worker_count):
    result = run(
        [
            str(APP),
            "--benchmark",
            app_scenario,
            str(task_count),
            str(worker_count),
        ]
    )
    scenario, workers, tasks, total_ms, tasks_per_second = result.stdout.strip().split(",")
    return {
        "scenario": scenario_name,
        "app_scenario": scenario,
        "worker_count": int(workers),
        "task_count": int(tasks),
        "total_completion_ms": int(total_ms),
        "tasks_per_second": float(tasks_per_second),
    }


def main():
    build()
    rows = []

    for scenario_name, app_scenario, task_count in SCENARIOS:
        for worker_count in WORKER_COUNTS:
            row = run_benchmark(scenario_name, app_scenario, task_count, worker_count)
            rows.append(row)
            print(
                f"{row['scenario']} workers={row['worker_count']} "
                f"time_ms={row['total_completion_ms']} "
                f"tasks_per_second={row['tasks_per_second']:.2f}"
            )

    with RESULTS.open("w", newline="") as file:
        writer = csv.DictWriter(
            file,
            fieldnames=[
                "scenario",
                "app_scenario",
                "worker_count",
                "task_count",
                "total_completion_ms",
                "tasks_per_second",
            ],
        )
        writer.writeheader()
        writer.writerows(rows)

    print(f"Wrote {RESULTS}")


if __name__ == "__main__":
    main()
