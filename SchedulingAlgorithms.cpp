// Batch Scheduling Algorithms Simulation in C++
// Includes FIFO, SJF, and SRT algorithms
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <iomanip>
#include <climits>
using namespace std;

struct Process {
    int pid, arrival, total, remaining, turnaround;
    bool active;
};

// Utility to generate processes
vector<Process> generate_processes(int n, int k, double d, double v) {
    default_random_engine gen(random_device{}());
    uniform_int_distribution<int> arrival_dist(0, k);
    normal_distribution<double> cpu_dist(d, v);

    vector<Process> procs;
    for (int i = 0; i < n; ++i) {
        int arrival = arrival_dist(gen);
        int total = max(1, (int)round(cpu_dist(gen)));
        procs.push_back({i, arrival, total, total, 0, false});
    }
    return procs;
}

// FIFO Algorithm
void simulate_FIFO(vector<Process> procs) {
    int t = 0;
    int n = procs.size();
    vector<bool> finished(n, false);

    while (count(finished.begin(), finished.end(), false) > 0) {
        int idx = -1;
        for (int i = 0; i < n; ++i) {
            if (!finished[i] && procs[i].arrival <= t) {
                idx = i;
                break;
            }
        }
        if (idx == -1) { t++; continue; }

        while (procs[idx].remaining > 0) {
            procs[idx].remaining--;
            t++;
        }
        procs[idx].turnaround = t - procs[idx].arrival;
        finished[idx] = true;
    }

    double ATT = 0;
    for (auto& p : procs) ATT += p.turnaround;
    ATT /= n;
    cout << "FIFO ATT = " << fixed << setprecision(2) << ATT << endl;
}

// SJF Algorithm
void simulate_SJF(vector<Process> procs) {
    int t = 0;
    int n = procs.size();
    vector<bool> finished(n, false);

    while (count(finished.begin(), finished.end(), false) > 0) {
        int idx = -1;
        int min_time = INT_MAX;
        for (int i = 0; i < n; ++i) {
            if (!finished[i] && procs[i].arrival <= t && procs[i].total < min_time) {
                min_time = procs[i].total;
                idx = i;
            }
        }
        if (idx == -1) { t++; continue; }

        while (procs[idx].remaining > 0) {
            procs[idx].remaining--;
            t++;
        }
        procs[idx].turnaround = t - procs[idx].arrival;
        finished[idx] = true;
    }

    double ATT = 0;
    for (auto& p : procs) ATT += p.turnaround;
    ATT /= n;
    cout << "SJF ATT = " << fixed << setprecision(2) << ATT << endl;
}

// SRT Algorithm
void simulate_SRT(vector<Process> procs) {
    int t = 0;
    int n = procs.size();
    vector<bool> finished(n, false);

    while (count(finished.begin(), finished.end(), false) > 0) {
        int idx = -1;
        int min_remaining = INT_MAX;
        for (int i = 0; i < n; ++i) {
            if (!finished[i] && procs[i].arrival <= t && procs[i].remaining < min_remaining && procs[i].remaining > 0) {
                min_remaining = procs[i].remaining;
                idx = i;
            }
        }
        if (idx == -1) { t++; continue; }

        procs[idx].remaining--;
        t++;
        if (procs[idx].remaining == 0) {
            procs[idx].turnaround = t - procs[idx].arrival;
            finished[idx] = true;
        }
    }

    double ATT = 0;
    for (auto& p : procs) ATT += p.turnaround;
    ATT /= n;
    cout << "SRT ATT = " << fixed << setprecision(2) << ATT << endl;
}

int main() {
    int n = 50; // number of processes
    int k = 1000; // max arrival time
    double d = 20.0, v = 5.0; // mean and stddev for total CPU time

    auto procs = generate_processes(n, k, d, v);
    simulate_FIFO(procs);
    simulate_SJF(procs);
    simulate_SRT(procs);
    return 0;
}
