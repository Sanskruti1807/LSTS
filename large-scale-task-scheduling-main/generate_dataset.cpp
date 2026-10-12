
#include <iostream>
#include <fstream>
#include <random>

using namespace std;

int main()
{
    int n = 1000000;

    mt19937 rng(42);
    uniform_int_distribution<int> dist(1, 100);

    ofstream file("tasks.csv");

    if (!file.is_open())
    {
        cout << "Error creating CSV file.\n";
        return 1;
    }

    file << "task_id,processing_time\n";

    for (int i = 0; i < n; i++)
    {
        int processingTime = dist(rng);

        file << i + 1 << ","
             << processingTime << "\n";
    }

    file.close();

    cout << "Dataset generated successfully!\n";
    cout << "Total tasks: " << n << "\n";

    return 0;
}
