// Parallel Large-Scale Task Scheduling using OpenMP
// Strategy: "Partitioned LPT"
//   1. Sort tasks (descending) with a parallel sort.
//   2. Resources are split into T contiguous groups (T = number of threads).
//   3. Sorted tasks are dealt to groups in snake order (0..T-1, T-1..0, ...)
//      so every group receives a similar total workload.
//   4. Each thread runs the same greedy LPT on ITS OWN tasks and ITS OWN resources
//      -> no shared writes, no locks, no race conditions.
// Usage: ./par <numTasks> <numResources> <numThreads> [tasks.csv] [timeColumnIndex(0-based)]
#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <climits>
#include <random>
#include <chrono>
#include <omp.h>
#ifdef _OPENMP
#if defined(__GNUC__) && !defined(__clang__)
#include <parallel/algorithm>
#define PSORT __gnu_parallel::sort
#else
#define PSORT std::sort
#endif
#endif

using namespace std;

struct Task
{
    int id;
    int processingTime;
};

struct Resource
{
    int id;
    long long currentLoad;
    vector<int> assignedTasks;
};

vector<Task> generateTasks(int n)
{
    vector<Task> tasks(n);
    mt19937 rng(42);
    uniform_int_distribution<int> dist(1, 100);
    for (int i = 0; i < n; i++)
    {
        tasks[i].id = i + 1;
        tasks[i].processingTime = dist(rng);
    }
    return tasks;
}

// Load tasks from the Cloud Task Scheduling CSV (or any CSV with a duration column).
// - Column is auto-detected from the header: "Task_Duration_sec" or "processing_time".
//   Otherwise the first column is used, or pass colIndex (0-based) to choose one.
// - Durations such as 35.18 s are stored as integers in units of 0.01 s (x100)
//   so no precision is lost; makespan values below are in these units.
// - If n is larger than the rows in the file, rows are reused cyclically so the
//   problem size can be scaled beyond the dataset size.
vector<Task> loadTasks(const string &file, int n, int colIndex = -1)
{
    vector<Task> rows;
    ifstream f(file);
    if (!f)
    {
        cerr << "Cannot open " << file << "\n";
        exit(1);
    }
    string line;
    int col = colIndex;
    double scale = 1.0;
    bool first = true;
    while (getline(f, line))
    {
        for (char &c : line)
        {
            if (c == '\r') c = ' ';
            else if (c == ';' || c == '\t') c = ',';
        }
        vector<string> fields;
        stringstream ss(line);
        string cell;
        while (getline(ss, cell, ','))
            fields.push_back(cell);
        if (fields.empty()) continue;

        if (first)
        {
            first = false;
            bool numeric = !fields[0].empty() &&
                           (isdigit((unsigned char)fields[0][0]) || fields[0][0] == '-');
            if (!numeric) // header row
            {
                if (col < 0)
                {
                    col = 0;
                    for (size_t i = 0; i < fields.size(); i++)
                    {
                        if (fields[i].find("Task_Duration_sec") != string::npos)
                        {
                            col = i;
                            scale = 100.0;
                        }
                        else if (fields[i].find("processing_time") != string::npos)
                            col = i;
                    }
                }
                continue;
            }
            if (col < 0) col = 0;
        }
        if (col >= (int)fields.size()) continue;
        try
        {
            double v = stod(fields[col]) * scale;
            Task t;
            t.id = 0;
            t.processingTime = (int)max(1.0, floor(v + 0.5));
            rows.push_back(t);
        }
        catch (...) { }
    }
    if (rows.empty())
    {
        cerr << "No valid tasks found in " << file << "\n";
        exit(1);
    }
    size_t available = rows.size();
    if ((size_t)n > available)
        cerr << "Note: file has " << available << " tasks; reusing them cyclically to reach " << n << ".\n";
    vector<Task> tasks(n);
    for (int i = 0; i < n; i++)
    {
        tasks[i].id = i + 1;
        tasks[i].processingTime = rows[i % available].processingTime;
    }
    return tasks;
}

vector<Resource> parallelSchedule(vector<Task> &tasks, int m, int T)
{
    int n = tasks.size();

    // Step 1: parallel sort, largest first (id tie-breaker => deterministic)
    PSORT(tasks.begin(), tasks.end(),
          [](const Task &a, const Task &b)
          {
              if (a.processingTime != b.processingTime)
                  return a.processingTime > b.processingTime;
              return a.id < b.id;
          });

    vector<Resource> resources(m);
    for (int i = 0; i < m; i++)
    {
        resources[i].id = i + 1;
        resources[i].currentLoad = 0;
    }

    // Step 2: resource group boundaries (group g owns resources [rs[g], rs[g+1]))
    vector<int> rs(T + 1, 0);
    for (int g = 0; g < T; g++)
        rs[g + 1] = rs[g] + m / T + (g < m % T ? 1 : 0);

    // Step 3+4: each thread handles its own group independently
    #pragma omp parallel num_threads(T)
    {
        int g = omp_get_thread_num();
        int lo = rs[g], hi = rs[g + 1];

        // snake-order dealing: round r gives this group task index r*T + (r even ? g : T-1-g)
        for (long long r = 0;; r++)
        {
            long long k = r * T + ((r % 2 == 0) ? g : (T - 1 - g));
            if (k >= n)
            {
                if (r * T >= n) break;
                continue;
            }
            const Task &task = tasks[k];

            int minResource = lo;
            for (int j = lo + 1; j < hi; j++)
                if (resources[j].currentLoad < resources[minResource].currentLoad)
                    minResource = j;

            resources[minResource].assignedTasks.push_back(task.id);
            resources[minResource].currentLoad += task.processingTime;
        }
    }
    return resources;
}

void calculateMetrics(const vector<Resource> &resources, const vector<Task> &tasks,
                      int n, int m, int T, double executionTime)
{
    long long totalLoad = 0, maxLoad = 0, minLoad = LLONG_MAX, assignedCount = 0;
    #pragma omp parallel for reduction(+ : totalLoad, assignedCount) reduction(max : maxLoad) reduction(min : minLoad)
    for (int i = 0; i < m; i++)
    {
        totalLoad += resources[i].currentLoad;
        assignedCount += resources[i].assignedTasks.size();
        maxLoad = max(maxLoad, resources[i].currentLoad);
        minLoad = min(minLoad, resources[i].currentLoad);
    }

    long long inputTotal = 0;
    int maxTask = 0;
    for (const auto &t : tasks)
    {
        inputTotal += t.processingTime;
        maxTask = max(maxTask, t.processingTime);
    }
    double averageLoad = (double)totalLoad / m;
    double loadImbalance = ((double)(maxLoad - minLoad) / averageLoad) * 100;
    double lowerBound = max(averageLoad, (double)maxTask);
    bool valid = (assignedCount == n) && (totalLoad == inputTotal);

    cout << "\n========== RESULTS (OpenMP PARALLEL) ==========\n";
    cout << "Number of Tasks     : " << n << "\n";
    cout << "Number of Resources : " << m << "\n";
    cout << "Threads Used        : " << T << "\n";
    cout << "Execution Time      : " << executionTime << " ms\n";
    cout << "Makespan            : " << maxLoad << "\n";
    cout << "Lower Bound         : " << lowerBound << "\n";
    cout << "Makespan / LB       : " << maxLoad / lowerBound << "\n";
    cout << "Average Load        : " << averageLoad << "\n";
    cout << "Load Imbalance      : " << loadImbalance << "%\n";
    cout << "Valid (all assigned): " << (valid ? "YES" : "NO") << "\n";
    cout << "===============================================\n";
    cout << "CSV,par," << T << "," << n << "," << m << "," << executionTime << "," << maxLoad << "\n";
}

int main(int argc, char *argv[])
{
    int n, m, T;
    if (argc >= 4)
    {
        n = atoi(argv[1]);
        m = atoi(argv[2]);
        T = atoi(argv[3]);
    }
    else
    {
        cout << "Enter number of tasks: ";
        cin >> n;
        cout << "Enter number of resources: ";
        cin >> m;
        cout << "Enter number of threads: ";
        cin >> T;
    }
    if (T < 1) T = 1;
    if (T > m) T = m; // each thread needs at least one resource

    vector<Task> tasks = (argc >= 5) ? loadTasks(argv[4], n, (argc >= 6) ? atoi(argv[5]) : -1) : generateTasks(n);
    n = tasks.size();
    vector<Task> original = tasks;

    double start = omp_get_wtime();
    vector<Resource> resources = parallelSchedule(tasks, m, T);
    double executionTime = (omp_get_wtime() - start) * 1000.0;

    calculateMetrics(resources, original, n, m, T, executionTime);
    return 0;
}
