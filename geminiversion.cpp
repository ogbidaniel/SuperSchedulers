#include <iostream>
#include <vector>
#include <random>
#include <numeric> // For std::accumulate
#include <algorithm> // For std::sort, std::min_element
#include <cmath> // For std::round, std::max
#include <iomanip> // For std::fixed, std::setprecision
#include <limits> // For std::numeric_limits

// --- Process Structure ---
struct Process {
    int id;
    int arrivalTime;
    int totalCpuTime;     // T_i (original burst time)
    int remainingCpuTime; // R_i
    int completionTime;
    int turnaroundTime;   // TT_i
    bool active;          // Is the process competing (arrived but not finished)?

    // Constructor
    Process(int p_id = 0, int arr = 0, int cpu = 0) :
        id(p_id),
        arrivalTime(arr),
        totalCpuTime(cpu),
        remainingCpuTime(cpu),
        completionTime(-1), // Not completed yet
        turnaroundTime(-1), // Not calculated yet
        active(false) {} // Initially inactive until arrival time is met

    // For sorting primarily by a specific criteria, using ID as tie-breaker
    static bool compareArrival(const Process* a, const Process* b) {
        if (a->arrivalTime != b->arrivalTime) {
            return a->arrivalTime < b->arrivalTime;
        }
        return a->id < b->id; // Tie-breaker
    }

    static bool compareTotalCpuTime(const Process* a, const Process* b) {
        if (a->totalCpuTime != b->totalCpuTime) {
            return a->totalCpuTime < b->totalCpuTime;
        }
        return a->id < b->id; // Tie-breaker
    }

    static bool compareRemainingCpuTime(const Process* a, const Process* b) {
        if (a->remainingCpuTime != b->remainingCpuTime) {
            return a->remainingCpuTime < b->remainingCpuTime;
        }
        return a->id < b->id; // Tie-breaker
    }
};

// --- Helper Function to Generate Processes ---
std::vector<Process> generateProcesses(int n, int k, double d, double v, std::mt19937& gen) {
    std::vector<Process> processes;
    std::uniform_int_distribution<> arrival_dist(0, k);
    std::normal_distribution<> cpu_dist(d, v);

    for (int i = 0; i < n; ++i) {
        int arrivalTime = arrival_dist(gen);
        // Ensure CPU time is at least 1
        int totalCpuTime = std::max(1, static_cast<int>(std::round(cpu_dist(gen))));
        processes.emplace_back(i, arrivalTime, totalCpuTime);
    }
    return processes;
}

// --- Simulation Core Logic (Common parts refactored if desired, or separate for clarity) ---

// --- FIFO Simulation ---
double simulateFIFO(std::vector<Process> processes, int n) {
    int currentTime = 0;
    int completedProcesses = 0;
    std::vector<Process*> readyQueue; // Pointers to processes ready to run

    while (completedProcesses < n) {
        // 1. Check for new arrivals
        for (Process& p : processes) {
            if (!p.active && p.arrivalTime <= currentTime && p.completionTime == -1) {
                p.active = true;
                readyQueue.push_back(&p);
                // Sort ready queue by arrival time (FIFO) upon insertion
                std::sort(readyQueue.begin(), readyQueue.end(), Process::compareArrival);
            }
        }

        // 2. Handle Idle Time
        if (readyQueue.empty()) {
            // Advance time to the next arrival if no process is ready
             int nextArrivalTime = std::numeric_limits<int>::max();
             bool foundNext = false;
             for(const auto& p : processes) {
                 if(p.completionTime == -1 && p.arrivalTime > currentTime) { // Find earliest future arrival
                     nextArrivalTime = std::min(nextArrivalTime, p.arrivalTime);
                     foundNext = true;
                 }
             }
             // If no processes arrived yet and queue is empty, go to first arrival
             // If some completed and queue empty, go to next arrival
             if(foundNext) {
                 currentTime = nextArrivalTime;
             } else if (completedProcesses < n) {
                 // Should not happen if generateProcesses guarantees arrivals <= k,
                 // but as a safeguard if queue is empty and no future arrivals found,
                 // break or increment time (let's increment).
                 currentTime++;
             } else {
                 break; // All done
             }
            continue; // Re-check arrivals at the new time
        }

        // 3. Choose Process (FIFO: the one at the front of the sorted readyQueue)
        Process* currentProcess = readyQueue.front();
        readyQueue.erase(readyQueue.begin()); // Remove from ready queue *conceptually* - it will run to completion

        // 4. Simulate Execution (Non-preemptive)
        // No need to check arrival time here, it was handled above.
        // The process runs for its *entire* burst time.
        currentTime += currentProcess->totalCpuTime; // Time jumps to completion
        currentProcess->remainingCpuTime = 0;


        // 5. Handle Completion
        currentProcess->completionTime = currentTime;
        currentProcess->turnaroundTime = currentProcess->completionTime - currentProcess->arrivalTime;
        currentProcess->active = false; // No longer active
        completedProcesses++;

        // Crucially, after a process completes at the *new* currentTime,
        // check for any processes that might have arrived *during* its execution.
        // This loop ensures that newly arrived processes are added *before* the next cycle.
        // (Note: This check is simplified here; a more robust implementation might
        // check arrivals *at each time step* if events could occur mid-burst, but
        // for basic FIFO/SJF/SRT simulation step-by-step or burst-by-burst is common)
        // Let's refine the loop: check arrivals *before* choosing the next process.
        // The above loop does this.
    }

    // Calculate Average Turnaround Time (ATT)
    double totalTurnaroundTime = 0;
    for (const auto& p : processes) {
         if (p.turnaroundTime != -1) { // Ensure it completed
              totalTurnaroundTime += p.turnaroundTime;
         } else {
             // This indicates an error or incomplete simulation
             std::cerr << "Warning: Process " << p.id << " did not complete." << std::endl;
         }
    }
    return (completedProcesses > 0) ? (totalTurnaroundTime / completedProcesses) : 0.0;
}


// --- SJF Simulation (Non-Preemptive) ---
double simulateSJF(std::vector<Process> processes, int n) {
    int currentTime = 0;
    int completedProcesses = 0;
    std::vector<Process*> readyQueue; // Pointers to processes ready to run
    Process* runningProcess = nullptr; // No process running initially

    while (completedProcesses < n) {
         // 1. Check for new arrivals and add to ready queue
        for (Process& p : processes) {
            // Add if: arrived, not yet completed, not already running or in ready queue
            if (p.arrivalTime <= currentTime && p.completionTime == -1 && !p.active) {
                 // Check if it's the currently running process (shouldn't be if completionTime == -1, but safety)
                 if (&p != runningProcess) {
                    p.active = true; // Mark as active/ready
                    readyQueue.push_back(&p);
                 }
            }
        }

        // 2. If CPU is idle and ready queue is not empty, choose next process
        if (runningProcess == nullptr && !readyQueue.empty()) {
            // Sort ready queue by total CPU time (SJF)
            std::sort(readyQueue.begin(), readyQueue.end(), Process::compareTotalCpuTime);
            runningProcess = readyQueue.front(); // Select shortest job
            readyQueue.erase(readyQueue.begin()); // Remove from ready queue
            // runningProcess->active remains true (it's active because it's running)
        }

        // 3. Handle Idle Time or Advance Time
        if (runningProcess == nullptr) {
            // CPU is idle, and ready queue is empty. Advance time.
             int nextArrivalTime = std::numeric_limits<int>::max();
             bool foundNext = false;
             for(const auto& p : processes) {
                 // Look for the earliest arrival among non-completed processes
                 if(p.completionTime == -1 && p.arrivalTime > currentTime) {
                     nextArrivalTime = std::min(nextArrivalTime, p.arrivalTime);
                     foundNext = true;
                 }
             }
             if (foundNext) {
                currentTime = nextArrivalTime; // Jump to the next arrival time
             } else if (completedProcesses < n) {
                 // No running process, nothing in ready queue, no future arrivals pending?
                 // This might mean all remaining processes have arrived but simulation stuck.
                 // Or simply advance time by 1 if nothing else to do.
                 // If readyQueue was checked correctly, this shouldn't happen unless all processes done.
                 if (!readyQueue.empty()) {
                     // This case should ideally not be reached if logic is correct
                      currentTime++;
                 } else {
                     // Break if truly nothing left to do.
                     if(completedProcesses == n) break;
                     // Otherwise, advance time by 1 if stuck? Or maybe error state.
                     // Let's try advancing time if arrivals check was exhaustive
                     currentTime++;
                 }
             } else {
                 break; // All completed
             }
            continue; // Re-evaluate at the new time
        }

        // 4. Simulate one time unit for the running process
        currentTime++;
        if (runningProcess) { // Ensure runningProcess is not null
             runningProcess->remainingCpuTime--;

             // 5. Handle Completion
             if (runningProcess->remainingCpuTime == 0) {
                 runningProcess->completionTime = currentTime;
                 runningProcess->turnaroundTime = runningProcess->completionTime - runningProcess->arrivalTime;
                 runningProcess->active = false; // No longer active/running
                 completedProcesses++;
                 runningProcess = nullptr; // CPU becomes idle
             }
        } else {
             // Should not happen if idle time handling is correct
             // currentTime++; // Already incremented above loop
        }

    } // End while loop

    // Calculate Average Turnaround Time (ATT)
    double totalTurnaroundTime = 0;
    int actualCompleted = 0;
    for (const auto& p : processes) {
        if (p.turnaroundTime != -1) {
            totalTurnaroundTime += p.turnaroundTime;
            actualCompleted++;
        } else {
             // std::cerr << "Warning: SJF Process " << p.id << " did not complete." << std::endl;
        }
    }
     if (actualCompleted != n) {
          // std::cerr << "Warning: SJF completed " << actualCompleted << "/" << n << " processes." << std::endl;
     }
    return (actualCompleted > 0) ? (totalTurnaroundTime / actualCompleted) : 0.0;
}


// --- SRT Simulation (Preemptive) ---
double simulateSRT(std::vector<Process> processes, int n) {
    int currentTime = 0;
    int completedProcesses = 0;
    std::vector<Process*> readyQueue; // Pointers to processes ready to run (or potentially run)
    Process* runningProcess = nullptr; // Currently running process

    while (completedProcesses < n) {
        // 1. Check for new arrivals and add to ready queue
        for (Process& p : processes) {
             // Add if: arrived, not yet completed, and not already in ready queue or running
            if (p.arrivalTime <= currentTime && p.completionTime == -1 && !p.active) {
                 bool alreadyInQueue = false;
                 for(const auto* rp : readyQueue) if (rp->id == p.id) alreadyInQueue = true;
                 if (!alreadyInQueue && (&p != runningProcess)) {
                    p.active = true; // Mark as ready/active
                    readyQueue.push_back(&p);
                 }
            }
        }

        // 2. Add the currently running process back to ready queue if it exists (for preemption check)
        if (runningProcess) {
            readyQueue.push_back(runningProcess);
            runningProcess = nullptr; // Temporarily make CPU idle for selection
        }

        // 3. Choose Process (SRT: shortest *remaining* time)
        if (!readyQueue.empty()) {
            // Sort by remaining CPU time
            std::sort(readyQueue.begin(), readyQueue.end(), Process::compareRemainingCpuTime);
            runningProcess = readyQueue.front(); // Select shortest remaining time job
            readyQueue.erase(readyQueue.begin()); // Remove it from ready queue (it's now running)
        }

        // 4. Handle Idle Time
        if (runningProcess == nullptr) {
            // CPU is idle, and ready queue is empty. Advance time.
            int nextArrivalTime = std::numeric_limits<int>::max();
            bool foundNext = false;
            for(const auto& p : processes) {
                 if(p.completionTime == -1 && p.arrivalTime > currentTime) {
                     nextArrivalTime = std::min(nextArrivalTime, p.arrivalTime);
                     foundNext = true;
                 }
             }
            if (foundNext) {
                currentTime = nextArrivalTime;
             } else if (completedProcesses < n) {
                 // If queue is empty and no future arrivals, maybe advance time by 1?
                 currentTime++;
             } else {
                 break; // All completed
             }
            continue; // Re-evaluate at the new time
        }

        // 5. Simulate one time unit
        currentTime++;
        if (runningProcess) { // Check again, as it might be null after idle handling
            runningProcess->remainingCpuTime--;

            // 6. Handle Completion
            if (runningProcess->remainingCpuTime == 0) {
                runningProcess->completionTime = currentTime;
                runningProcess->turnaroundTime = runningProcess->completionTime - runningProcess->arrivalTime;
                runningProcess->active = false; // No longer active/competing
                completedProcesses++;
                runningProcess = nullptr; // CPU becomes idle for the next cycle's selection
            }
            // Note: If not completed, runningProcess might be preempted in the next cycle
            // because it's added back to readyQueue in step 2.
        } else {
             // Should not be reached if logic is correct
            // currentTime++; // Already incremented
        }
    }

    // Calculate Average Turnaround Time (ATT)
    double totalTurnaroundTime = 0;
    int actualCompleted = 0;
    for (const auto& p : processes) {
        if (p.turnaroundTime != -1) {
            totalTurnaroundTime += p.turnaroundTime;
            actualCompleted++;
        } else {
           // std::cerr << "Warning: SRT Process " << p.id << " did not complete." << std::endl;
        }
    }
     if (actualCompleted != n) {
         // std::cerr << "Warning: SRT completed " << actualCompleted << "/" << n << " processes." << std::endl;
     }
    return (actualCompleted > 0) ? (totalTurnaroundTime / actualCompleted) : 0.0;
}


// --- Main Driver ---
int main() {
    // Simulation Parameters
    int n = 50;              // Number of processes
    int k = 200;            // Max arrival time
    double v_percentage = 0.3; // Standard deviation as a percentage of d (e.g., 30%)

    // Average CPU times to test (d values)
    // Select values relative to k/n (average arrival interval)
    double avg_arrival_interval = static_cast<double>(k) / n; // = 1000 / 50 = 20
    std::vector<double> d_values;
    // Examples: Much smaller, around, and much larger than avg_arrival_interval
    d_values.push_back(avg_arrival_interval * 0.1);  // d = 2 (Low contention)
    d_values.push_back(avg_arrival_interval * 0.5);  // d = 10
    d_values.push_back(avg_arrival_interval * 1.0);  // d = 20 (Moderate contention)
    d_values.push_back(avg_arrival_interval * 2.0);  // d = 40
    d_values.push_back(avg_arrival_interval * 5.0);  // d = 100 (High contention)
    d_values.push_back(avg_arrival_interval * 10.0); // d = 200 (Very high contention)


    // Random Number Generator Setup
    std::random_device rd;
    std::mt19937 gen(rd()); // Mersenne Twister engine seeded with random_device

    // Output Header for Results (Tab-separated for easy plotting)
    std::cout << "d\tATT_FIFO\td/ATT_FIFO\tATT_SJF\td/ATT_SJF\tATT_SRT\td/ATT_SRT\n";
    std::cout << std::fixed << std::setprecision(4); // Format output

    // Run simulation for each d value
    for (double d : d_values) {
        double v = d * v_percentage;
        if (v < 1.0) v = 1.0; // Ensure minimum standard deviation

        // Generate ONE set of processes for this d
        std::vector<Process> initial_processes = generateProcesses(n, k, d, v, gen);

        // --- Run Simulations (pass copies to keep initial_processes unchanged) ---
        double att_fifo = simulateFIFO(initial_processes, n);
        double att_sjf = simulateSJF(initial_processes, n);
        double att_srt = simulateSRT(initial_processes, n);

        // --- Output Results ---
        std::cout << d << "\t"
                  << att_fifo << "\t" << (att_fifo > 0 ? d / att_fifo : 0.0) << "\t"
                  << att_sjf << "\t" << (att_sjf > 0 ? d / att_sjf : 0.0) << "\t"
                  << att_srt << "\t" << (att_srt > 0 ? d / att_srt : 0.0) << "\n";
    }

    return 0;
}
