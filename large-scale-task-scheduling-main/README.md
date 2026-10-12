# Large-Scale Task Scheduling

## 1. Project Overview

This project implements a task scheduling system that distributes tasks across multiple resources while attempting to balance their workloads.

It compares two approaches:

- **Sequential scheduling:** Uses a Longest Processing Time (LPT)-first greedy strategy to assign tasks to the resource with the lowest current workload.
- **Parallel scheduling:** Uses OpenMP to schedule chunks of the sorted task list concurrently and merge the results.

The project evaluates performance for increasing task counts and different thread configurations using execution time, makespan, speedup, and parallel efficiency.

## 2. Repository Structure

```text
large-scale-task-scheduling/
├── sequential.cpp
├── parallel.cpp
├── tasks.csv
├── README.md
├── .gitignore
├── results/
│   ├── execution_time_problem_size.png
│   ├── thread_scaling.png
│   └── speedup_threads.png
```

The `results/` folder contains performance graphs.

## 3. Requirements

- C++ compiler (GCC/g++)
- GCC with OpenMP support for the parallel implementation
- Visual Studio Code or another C++ development environment
- The dataset `tasks.csv`

The project was developed using the MSYS2 UCRT64 GCC toolchain on Windows.

Check your compiler installation:

```powershell
g++ --version
```

## 4. Compilation and Execution

Run commands from the project directory containing the source files and dataset.

### Sequential implementation

Compile:

```powershell
g++ sequential.cpp -o sequential.exe
```

Run:

```powershell
.\sequential.exe
```

Enter the requested number of tasks and resources when prompted.

### Parallel implementation

Compile with OpenMP enabled:

```powershell
g++ -fopenmp parallel.cpp -o parallel.exe
```

Run:

```powershell
.\parallel.exe
```

Enter the requested number of tasks, resources, and threads when prompted.

Ensure that the CSV dataset is accessible from the program's working directory before execution.

## 5. Input Format

The programs read tasks from `tasks.csv`.

Expected format:

```csv
task_id,processing_time
1,38
2,80
3,96
4,19
```

Each row represents one task:

- `task_id`: Unique task identifier.
- `processing_time`: Processing time associated with the task.

The supplied dataset contains 1,000,000 tasks, with processing times generated in the range 1–100 using a fixed random seed.

Both programs load the dataset before the scheduling timer starts, so CSV loading time is excluded from the reported scheduling execution time.

Use the same dataset for sequential and parallel comparisons to maintain consistent inputs. Do not regenerate the dataset unless you intend to conduct experiments with a new input.

## 6. Developer Guide

### Modifying the Scheduling Logic

- The sequential scheduling logic is implemented in `sequential.cpp`.
- The parallel scheduling logic is implemented in `parallel.cpp`.
- Preserve consistent task representation and metric calculations when modifying either implementation.
- If the scheduling algorithm changes, review both implementations to ensure the comparison remains meaningful.

### Updating the Dataset

- Preserve the CSV header and column order unless the CSV parser is updated.
- Ensure task IDs and processing times are valid.
- Ensure the dataset contains enough rows for the requested task count.
- Use the same dataset when comparing sequential and parallel performance.

### Modifying Parallel Execution

The parallel implementation uses OpenMP and thread-local workload data before merging results.

When modifying it, consider thread safety, synchronization, workload distribution, and merging overhead. Compile the parallel program with the `-fopenmp` flag.

### Testing Changes

After modifying the code:

1. Compile both implementations.
2. Test with a small task count first.
3. Check task assignments, resource workloads, makespan, and load imbalance.
4. Use the same input data and configuration when comparing implementations.
5. Update the report and graphs if new performance experiments are conducted.



