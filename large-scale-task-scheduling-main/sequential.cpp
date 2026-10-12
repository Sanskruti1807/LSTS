
#include <iostream>
#include <vector>
#include <algorithm>
#include <climits>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>

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

// Read tasks from CSV
vector<Task> loadTasksFromCSV(const string &filename, int n)
{
    vector<Task> tasks;
    ifstream file(filename);

    if (!file.is_open())
    {
        cout << "Error: Could not open CSV file!\n";
        return tasks;
    }

    string line;

    // Skip header
    getline(file, line);

    // Read the first n tasks
    while (tasks.size() < static_cast<size_t>(n) &&
           getline(file, line))
    {
        stringstream ss(line);
        string idText, timeText;

        if (!getline(ss, idText, ',') ||
            !getline(ss, timeText, ','))
        {
            cout << "Error: Invalid CSV row.\n";
            return {};
        }

        try
        {
            Task task;
            task.id = stoi(idText);
            task.processingTime = stoi(timeText);
            tasks.push_back(task);
        }
        catch (...)
        {
            cout << "Error: Invalid data in CSV.\n";
            return {};
        }
    }

    file.close();

    if (tasks.size() != static_cast<size_t>(n))
    {
        cout << "Error: CSV contains fewer tasks than requested.\n";
        return {};
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
        resources[minResource].currentLoad += task.processingTime;
    }

    return resources;
}

// Calculate and display performance metrics
void calculateMetrics(const vector<Resource> &resources,
                      int n, int m, double executionTime)
{
    long long totalLoad = 0;
    long long maxLoad = 0;
    long long minLoad = LLONG_MAX;

    for (const auto &resource : resources)
    {
        totalLoad += resource.currentLoad;
        maxLoad = max(maxLoad, resource.currentLoad);
        minLoad = min(minLoad, resource.currentLoad);
    }

    double averageLoad = (double)totalLoad / m;
    double loadImbalance =
        ((double)(maxLoad - minLoad) / averageLoad) * 100;

    cout << "\n========== RESULTS ==========\n";
    cout << "Number of Tasks     : " << n << "\n";
    cout << "Number of Resources : " << m << "\n";
    cout << "Execution Time      : " << executionTime << " ms\n";
    cout << "Makespan            : " << maxLoad << "\n";
    cout << "Average Load        : " << averageLoad << "\n";
    cout << "Load Imbalance      : " << loadImbalance << "%\n";
    cout << "=============================\n";
}

int main()
{
    int n, m;

    cout << "Enter number of tasks (max 1000000): ";
    cin >> n;

    cout << "Enter number of resources: ";
    cin >> m;

    if (!cin || n <= 0 || n > 1000000 || m <= 0)
    {
        cout << "Invalid input.\n";
        return 1;
    }

    // Load input BEFORE timing the scheduler
    vector<Task> tasks =
        loadTasksFromCSV("tasks.csv", n);

    if (tasks.size() != static_cast<size_t>(n))
    {
        return 1;
    }

    // Start timing only the scheduling algorithm
    auto start = chrono::high_resolution_clock::now();

    vector<Resource> resources = sequentialSchedule(tasks, m);

    auto end = chrono::high_resolution_clock::now();

    double executionTime =
        chrono::duration<double, milli>(end - start).count();

    calculateMetrics(resources, n, m, executionTime);

    return 0;
}
