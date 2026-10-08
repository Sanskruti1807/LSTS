// Sequential Large-Scale Task Scheduling (Greedy LPT: Longest Processing Time first)
// Usage: ./seq <numTasks> <numResources> [tasks.csv] [timeColumnIndex(0-based)]
//   If tasks.csv is given, tasks are read from it; otherwise generated with seed 42.
#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
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

// Sequential greedy scheduling
vector<Resource> sequentialSchedule(vector<Task> &tasks, int m)
{
    // Sort tasks from largest to smallest (id as tie-breaker => deterministic)
    sort(tasks.begin(), tasks.end(),
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

    // Assign each task to the least-loaded resource
    for (const auto &task : tasks)
    {
        int minResource = 0;
        for (int j = 1; j < m; j++)
            if (resources[j].currentLoad < resources[minResource].currentLoad)
                minResource = j;

        resources[minResource].assignedTasks.push_back(task.id);
        resources[minResource].currentLoad += task.processingTime;
    }
    return resources;
}

// Calculate and display performance metrics
void calculateMetrics(const vector<Resource> &resources, const vector<Task> &tasks,
                      int n, int m, double executionTime)
{
    long long totalLoad = 0, maxLoad = 0, minLoad = LLONG_MAX, assignedCount = 0;
    for (const auto &r : resources)
    {
        totalLoad += r.currentLoad;
        maxLoad = max(maxLoad, r.currentLoad);
        minLoad = min(minLoad, r.currentLoad);
        assignedCount += r.assignedTasks.size();
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
    double lowerBound = max(averageLoad, (double)maxTask); // optimal makespan >= this
    bool valid = (assignedCount == n) && (totalLoad == inputTotal);

    cout << "\n========== RESULTS (SEQUENTIAL) ==========\n";
    cout << "Number of Tasks     : " << n << "\n";
    cout << "Number of Resources : " << m << "\n";
    cout << "Execution Time      : " << executionTime << " ms\n";
    cout << "Makespan            : " << maxLoad << "\n";
    cout << "Lower Bound         : " << lowerBound << "\n";
    cout << "Makespan / LB       : " << maxLoad / lowerBound << "\n";
    cout << "Average Load        : " << averageLoad << "\n";
    cout << "Load Imbalance      : " << loadImbalance << "%\n";
    cout << "Valid (all assigned): " << (valid ? "YES" : "NO") << "\n";
    cout << "==========================================\n";
    // machine-readable line for result collection
    cout << "CSV,seq,1," << n << "," << m << "," << executionTime << "," << maxLoad << "\n";
}

int main(int argc, char *argv[])
{
    int n, m;
    if (argc >= 3)
    {
        n = atoi(argv[1]);
        m = atoi(argv[2]);
    }
    else
    {
        cout << "Enter number of tasks: ";
        cin >> n;
        cout << "Enter number of resources: ";
        cin >> m;
    }

    vector<Task> tasks = (argc >= 4) ? loadTasks(argv[3], n, (argc >= 5) ? atoi(argv[4]) : -1) : generateTasks(n);
    n = tasks.size();
    vector<Task> original = tasks; // kept only for validation

    auto start = chrono::high_resolution_clock::now();
    vector<Resource> resources = sequentialSchedule(tasks, m);
    auto end = chrono::high_resolution_clock::now();

    double executionTime = chrono::duration<double, milli>(end - start).count();
    calculateMetrics(resources, original, n, m, executionTime);
    return 0;
}
