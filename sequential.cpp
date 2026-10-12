#include <iostream>
#include <vector>
#include <algorithm>
#include <climits>
#include <random>
#include <chrono>

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

// Generate the same tasks every time using seed 42
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

// Sequential greedy scheduling
vector<Resource> sequentialSchedule(vector<Task> &tasks, int m)
{

    // Sort tasks from largest to smallest
    sort(tasks.begin(), tasks.end(),
         [](const Task &a, const Task &b)
         {
             return a.processingTime > b.processingTime;
         });

    vector<Resource> resources(m);

    for (int i = 0; i < m; i++)
    {
        resources[i].id = i + 1;
        resources[i].currentLoad = 0;
    }

    // Assign each task to the least-loaded resource
    for (const auto &task : tasks)
    {

        int minResource = 0;

        for (int j = 1; j < m; j++)
        {

            if (resources[j].currentLoad <
                resources[minResource].currentLoad)
            {

                minResource = j;
            }
        }

        resources[minResource].assignedTasks.push_back(task.id);

        resources[minResource].currentLoad +=
            task.processingTime;
    }

    return resources;
}

// Calculate and display performance metrics
void calculateMetrics(const vector<Resource> &resources,
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

        maxLoad = max(maxLoad,
                      resource.currentLoad);

        minLoad = min(minLoad,
                      resource.currentLoad);
    }

    double averageLoad =
        (double)totalLoad / m;

    double loadImbalance =
        ((double)(maxLoad - minLoad) / averageLoad) * 100;

    cout << "\n========== RESULTS ==========\n";

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

    cout << "=============================\n";
}

int main()
{

    int n, m;

    cout << "Enter number of tasks: ";
    cin >> n;

    cout << "Enter number of resources: ";
    cin >> m;

    // Generate input data
    vector<Task> tasks = generateTasks(n);

    // Start timing ONLY the scheduling algorithm
    auto start =
        chrono::high_resolution_clock::now();

    // Run sequential scheduler
    vector<Resource> resources =
        sequentialSchedule(tasks, m);

    // Stop timing
    auto end =
        chrono::high_resolution_clock::now();

    double executionTime =
        chrono::duration<double, milli>(end - start).count();

    // Display results
    calculateMetrics(
        resources,
        n,
        m,
        executionTime);

    return 0;
}