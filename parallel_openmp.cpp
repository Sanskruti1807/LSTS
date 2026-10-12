#include <iostream>
#include <vector>
#include <algorithm>
#include <climits>
#include <random>
#include <chrono>
#include <omp.h>

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

// Generate the same tasks every time
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

// Parallel chunk-based LPT scheduling
vector<Resource> parallelSchedule(vector<Task> &tasks, int m)
{

    // Sort tasks from largest to smallest
    sort(tasks.begin(), tasks.end(),
         [](const Task &a, const Task &b)
         {
             return a.processingTime > b.processingTime;
         });

    int numThreads = omp_get_max_threads();

    // Each thread has its own resource loads
    vector<vector<long long>> localLoads(
        numThreads,
        vector<long long>(m, 0));

    // Each thread also stores its own assignments
    vector<vector<vector<int>>> localTasks(
        numThreads,
        vector<vector<int>>(m));

// Parallel scheduling
#pragma omp parallel
    {
        int tid = omp_get_thread_num();

        int start =
            (tid * tasks.size()) / numThreads;

        int end =
            ((tid + 1) * tasks.size()) / numThreads;

        // Each thread schedules its own chunk
        for (int i = start; i < end; i++)
        {

            int minResource = 0;

            // Find least-loaded LOCAL resource
            for (int j = 1; j < m; j++)
            {

                if (localLoads[tid][j] <
                    localLoads[tid][minResource])
                {

                    minResource = j;
                }
            }

            // Assign task locally
            localTasks[tid][minResource].push_back(
                tasks[i].id);

            localLoads[tid][minResource] +=
                tasks[i].processingTime;
        }
    }

    // Merge results from all threads
    vector<Resource> resources(m);

    for (int i = 0; i < m; i++)
    {

        resources[i].id = i + 1;
        resources[i].currentLoad = 0;
    }

    for (int t = 0; t < numThreads; t++)
    {

        for (int r = 0; r < m; r++)
        {

            resources[r].currentLoad +=
                localLoads[t][r];

            resources[r].assignedTasks.insert(
                resources[r].assignedTasks.end(),
                localTasks[t][r].begin(),
                localTasks[t][r].end());
        }
    }

    return resources;
}

// Calculate performance metrics
void calculateMetrics(
    const vector<Resource> &resources,
    int n,
    int m,
    double executionTime)
{

    long long totalLoad = 0;
    long long maxLoad = 0;
    long long minLoad = LLONG_MAX;

    for (const auto &resource : resources)
    {

        totalLoad += resource.currentLoad;

        maxLoad = max(
            maxLoad,
            resource.currentLoad);

        minLoad = min(
            minLoad,
            resource.currentLoad);
    }

    double averageLoad =
        (double)totalLoad / m;

    double loadImbalance =
        ((double)(maxLoad - minLoad) / averageLoad) * 100;

    cout << "\n========== PARALLEL RESULTS ==========\n";

    cout << "Number of Tasks     : "
         << n << "\n";

    cout << "Number of Resources : "
         << m << "\n";

    cout << "Execution Time      : "
         << executionTime << " ms\n";

    cout << "Makespan            : "
         << maxLoad << "\n";

    cout << "Average Load        : "
         << averageLoad << "\n";

    cout << "Load Imbalance      : "
         << loadImbalance << "%\n";

    cout << "======================================\n";
}

int main()
{

    int n, m, threads;

    cout << "Enter number of tasks: ";
    cin >> n;

    cout << "Enter number of resources: ";
    cin >> m;

    cout << "Enter number of threads: ";
    cin >> threads;

    omp_set_num_threads(threads);

    // Generate input
    vector<Task> tasks = generateTasks(n);

    // Start timing
    auto start =
        chrono::high_resolution_clock::now();

    // Run parallel scheduler
    vector<Resource> resources =
        parallelSchedule(tasks, m);

    // Stop timing
    auto end =
        chrono::high_resolution_clock::now();

    double executionTime =
        chrono::duration<double, milli>(end - start).count();

    calculateMetrics(
        resources,
        n,
        m,
        executionTime);

    return 0;
}