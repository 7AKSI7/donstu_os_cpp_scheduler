#include "fcfs.h"
#include "sjf.h"
#include "srtn.h"
#include "rr.h"
#include "priority.h"
#include "mlfq.h"
#include "simulator.h"
#include "testsets.h"
#include "hrrn.h"
#include "edf.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <functional>

std::vector<Process> makeTestSet() {
  std::vector<Process> procs;
  auto add = [&](int pid, const std::string& name, std::uint64_t arrival,
                 std::uint64_t burst, int priority,
                 std::vector<IoBlock> io = {}) {
    Process p;
    p.pid = pid;
    p.name = name;
    p.arrivalTime = arrival;
    p.burstTime = burst;
    p.remainingTime = burst;
    p.priority = priority;
    p.dynamicPriority = priority;
    p.ioBlocks = std::move(io);
    procs.push_back(p);
  };
  add(1, "P1", 0, 8, 3);
  add(2, "P2", 1, 4, 1);
  add(3, "P3", 2, 9, 4);
  add(4, "P4", 3, 5, 2);
  return procs;
}

std::vector<Process> makeIoTestSet() {
  std::vector<Process> procs;
  auto add = [&](int pid, const std::string& name,
                 std::uint64_t arrival, std::uint64_t burst,
                 int priority,
                 std::vector<IoBlock> io = {}) {
    Process p;
    p.pid = pid; p.name = name;
    p.arrivalTime = arrival;
    p.burstTime = burst;
    p.remainingTime = burst;
    p.priority = priority;
    p.dynamicPriority = priority;
    p.ioBlocks = std::move(io);
    procs.push_back(p);
  };
  add(1, "CPU1", 0, 20, 2);
  add(2, "IO1",  1,  2, 1, {{1, 5}, {1, 5}, {0, 0}});
  add(3, "IO2",  2,  2, 1, {{1, 5}, {1, 5}, {0, 0}});
  add(4, "CPU2", 3, 10, 3);
  return procs;
}

std::vector<Process> makeHrrnTestSet() {
  std::vector<Process> procs;
  auto add = [&](int pid, const std::string& name, std::uint64_t arrival,
                 std::uint64_t burst, int priority) {
    Process p;
    p.pid = pid;
    p.name = name;
    p.arrivalTime = arrival;
    p.burstTime = burst;
    p.remainingTime = burst;
    p.priority = priority;
    p.dynamicPriority = priority;
    procs.push_back(p);
  };
  add(1, "A",  0, 4, 1);
  add(2, "L",  1, 6, 1);
  add(3, "S1", 4, 2, 1);
  add(4, "S2", 5, 1, 1);
  return procs;
}

void printResult(const SimResult& r) {
  std::cout << std::left << std::setw(30) << r.algorithm
            << " | wait=" << std::setw(7) << std::fixed << std::setprecision(2) << r.avgWaiting
            << " | turn=" << std::setw(7) << r.avgTurnaround
            << " | resp=" << std::setw(7) << r.avgResponse
            << " | CPU=" << std::setw(6) << r.cpuUtilization << "%"
            << " | CS=" << std::setw(4) << r.contextSwitches
            << " | overhead=" << std::setw(5) << r.overheadPercent << "%"
            << " | done=" << r.throughput
            << "\n";
}

void printGantt(const SimResult& r) {
  std::cout << "Gantt (" << r.algorithm << "):\n";
  for (auto& [pid, span] : r.gantt) {
    std::cout << "  [" << span.first << "-" << span.second << ") ";
    if (pid == -1) std::cout << "IDLE\n";
    else if (pid == -2) std::cout << "CS\n";
    else std::cout << "P" << pid << "\n";
  }
  std::cout << "\n";
}

std::vector<Process> makeConvoySet() {
  std::vector<Process> procs;
  auto add = [&](int pid, const std::string& name, std::uint64_t arrival,
    std::uint64_t burst, int priority,
      std::vector<IoBlock> io = {}) {
        Process p;
        p.pid = pid; p.name = name;
        p.arrivalTime = arrival;
        p.burstTime = burst;
        p.remainingTime = burst;
        p.priority = priority;
        p.dynamicPriority = priority;
        p.ioBlocks = std::move(io);
        procs.push_back(p);
  };
  add(1, "CPU1", 0, 20, 2);
  add(2, "IO1", 1, 6, 1, {{1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4}});
  add(3, "IO2", 2, 6, 1, {{1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4}});
  add(4, "CPU2", 3, 12, 3);
  return procs;
}

using Factory = std::function<std::unique_ptr<Scheduler>(const std::vector<Process>&)>;

template <class T, class... Args>
Factory factory(Args... args) {
  return [=](const std::vector<Process>& s) {
    return std::make_unique<T>(s, args...);
  };
}

// metric: 0=wait, 1=turn, 2=resp, 3=CPU%
void runExperiment10(int metric) {
  std::vector<std::pair<std::string, Factory>> algos = {
    {"FCFS",       factory<FcfsScheduler>()},
    {"SJF",        factory<SjfScheduler>()},
    {"SRTN",       factory<SrtnScheduler>()},
    {"HRRN",       factory<HrrnScheduler>()},
    {"RR q=4",     factory<RrScheduler>(4)},
    {"Prio",       factory<PriorityScheduler>(false, false)},
    {"Prio+aging", factory<PriorityScheduler>(true, true)},
    {"MLFQ",       factory<MlfqScheduler>()},
  };

  const char* titles[4] = {
    "Average waiting time",
    "Average turnaround time",
    "Average response time",
    "CPU utilization (%)"
  };
  std::cout << "\n" << titles[metric] << "\n";

  const int sizes[5] = {12, 14, 16, 18, 20};

  std::cout << std::left << std::setw(12) << "Algorithm" << std::right;
  for (int i = 1; i <= 5; ++i)
    std::cout << std::setw(9) << ("set " + std::to_string(i));
  std::cout << std::setw(9) << "average" << "\n";
  std::cout << std::string(12 + 9 * 6, '-') << "\n";

  for (auto& [label, make] : algos) {
    std::cout << std::left << std::setw(12) << label << std::right;
    double sum = 0;
    for (unsigned seed = 1; seed <= 5; ++seed) {
      auto set = makeRandomSet(seed, sizes[seed - 1]);
      auto sched = make(set);
      SimResult r = runSimulation(*sched);
      double v = 0;
      switch (metric) {
        case 0: v = r.avgWaiting; break;
        case 1: v = r.avgTurnaround; break;
        case 2: v = r.avgResponse; break;
        case 3: v = r.cpuUtilization; break;
      }
      sum += v;
      std::cout << std::setw(9) << std::fixed << std::setprecision(2) << v;
    }
    std::cout << std::setw(9) << sum / 5 << "\n";
  }
}

std::vector<Process> makeEdfTestSet() {
  std::vector<Process> procs;
  auto add = [&](int pid, const std::string& name, std::uint64_t arrival,
                 std::uint64_t burst, std::uint64_t deadline) {
    Process p;
    p.pid = pid;
    p.name = name;
    p.arrivalTime = arrival;
    p.burstTime = burst;
    p.remainingTime = burst;
    p.deadline = deadline;
    procs.push_back(p);
  };
  // Классический пример, укладывающийся в сроки (загрузка 1.0)
  add(1, "T1", 0, 2, 5);
  add(2, "T2", 0, 3, 6);
  add(3, "T3", 0, 1, 3);
  return procs;
}

int main() {

      // ===== Задание 10: сохранить 5 случайных наборов =====
  {
    const int sizes[5] = {12, 14, 16, 18, 20};
    for (unsigned seed = 1; seed <= 5; ++seed) {
      saveSet("sets/set" + std::to_string(seed) + ".txt", makeRandomSet(seed, sizes[seed - 1]));
    }
    std::cout << "Saved sets/set1..5.txt\n";
  }

  std::cout << "Case 1: CPU-bound\n";
  {
    auto set1 = makeTestSet();
    std::vector<std::unique_ptr<Scheduler>> scheds;
    scheds.push_back(std::make_unique<FcfsScheduler>(set1));
    scheds.push_back(std::make_unique<SjfScheduler>(set1));
    scheds.push_back(std::make_unique<SrtnScheduler>(set1));
    scheds.push_back(std::make_unique<RrScheduler>(set1, 1));
    scheds.push_back(std::make_unique<RrScheduler>(set1, 2));
    scheds.push_back(std::make_unique<RrScheduler>(set1, 4));
    scheds.push_back(std::make_unique<PriorityScheduler>(set1, false, false));
    scheds.push_back(std::make_unique<PriorityScheduler>(set1, true, false));
    scheds.push_back(std::make_unique<PriorityScheduler>(set1, true, true));
    scheds.push_back(std::make_unique<MlfqScheduler>(set1));
    scheds.push_back(std::make_unique<HrrnScheduler>(set1));
    for (auto& s : scheds) printResult(runSimulation(*s));
  }

  std::cout << "\nCase 2. Mixed loading: I/O + CPU\n";
  {
    auto set2 = makeIoTestSet();
    std::vector<std::unique_ptr<Scheduler>> scheds;
    scheds.push_back(std::make_unique<FcfsScheduler>(set2));
    scheds.push_back(std::make_unique<SrtnScheduler>(set2));
    scheds.push_back(std::make_unique<RrScheduler>(set2, 4));
    scheds.push_back(std::make_unique<PriorityScheduler>(set2, true, true));
    scheds.push_back(std::make_unique<MlfqScheduler>(set2));
    for (auto& s : scheds) printResult(runSimulation(*s));
  }

  std::cout << "\nCase 3. HRRN vs SJF (starvation demo)\n";
  {
    auto set3 = makeHrrnTestSet();

    SjfScheduler sjf(set3);
    SimResult rSjf = runSimulation(sjf);
    printResult(rSjf);
    printGantt(rSjf);

    HrrnScheduler hrrn(set3);
    SimResult rHrrn = runSimulation(hrrn);
    printResult(rHrrn);
    printGantt(rHrrn);
  }

  // ===== Case 4: RR quantum vs switch cost =====
  std::cout << "\nCase 4. RR: quantum vs switch cost (makeTestSet)\n";
  {
    std::cout << std::left
              << std::setw(6) << "q"
              << std::setw(8) << "cost"
              << std::setw(10) << "wait"
              << std::setw(10) << "turn"
              << std::setw(10) << "resp"
              << std::setw(10) << "CPU%"
              << std::setw(12) << "overhead%"
              << std::setw(6) << "CS"
              << "\n";
    std::cout << std::string(72, '-') << "\n";

    for (std::uint64_t q : {1u, 2u, 4u, 8u}) {
      for (std::uint64_t cost : {0u, 1u}) {
        auto set = makeTestSet();
        RrScheduler rr(set, q);
        SimResult r = runSimulation(rr, 100000, cost);
        std::cout << std::left
                  << std::setw(6) << q
                  << std::setw(8) << cost
                  << std::setw(10) << std::fixed << std::setprecision(2) << r.avgWaiting
                  << std::setw(10) << r.avgTurnaround
                  << std::setw(10) << r.avgResponse
                  << std::setw(10) << r.cpuUtilization
                  << std::setw(12) << r.overheadPercent
                  << std::setw(6) << r.contextSwitches
                  << "\n";
      }
    }

    std::cout << "\nFCFS with switch cost:\n";
    std::cout << std::string(72, '-') << "\n";
    for (std::uint64_t cost : {0u, 1u}) {
      auto set = makeTestSet();
      FcfsScheduler fcfs(set);
      SimResult r = runSimulation(fcfs, 100000, cost);
      std::cout << std::left
                << std::setw(6) << "FCFS"
                << std::setw(8) << cost
                << std::setw(10) << std::fixed << std::setprecision(2) << r.avgWaiting
                << std::setw(10) << r.avgTurnaround
                << std::setw(10) << r.avgResponse
                << std::setw(10) << r.cpuUtilization
                << std::setw(12) << r.overheadPercent
                << std::setw(6) << r.contextSwitches
                << "\n";
    }
  }

  // ===== Гант RR q=2 без стоимости =====
  std::cout << "\n";
  {
    auto set = makeTestSet();
    RrScheduler rr(set, 2);
    SimResult r = runSimulation(rr);
    printGantt(r);
  }

  // ===== Гант RR q=2 cost=1 (видно CS) =====
  std::cout << "Gantt RR q=2 cost=1 (CS = переключение):\n";
  {
    auto set = makeTestSet();
    RrScheduler rr(set, 2);
    SimResult r = runSimulation(rr, 100000, 1);
    printGantt(r);
  }

  {
    auto set = makeTestSet();
    RrScheduler rr(set, 2);
    SimResult r = runSimulation(rr);
    std::cout << "\nПо процессам, RR (q=2):\n";
    printProcessTable(rr.processes());
  }

  {
    auto set = makeTestSet();
    saveSet("sets/set_basic.txt", set);
    std::vector<Process> loaded;
    if (!loadSet("sets/set_basic.txt", loaded)) {
      std::cerr << "Не удалось загрузить set_basic.txt\n";
      return 1;
    }
    FcfsScheduler a(set), b(loaded);
    printResult(runSimulation(a));
    printResult(runSimulation(b)); // строки должны совпасть
  }

  {
    auto set = makeTestSet();
    std::cout << "\nВлияние кванта в RR (набор makeTestSet)\n";
    std::cout << "q wait turn resp CS\n";
    for (std::uint64_t q : {1, 2, 4, 8, 16}) {
      RrScheduler rr(set, q);
      SimResult r = runSimulation(rr);
      std::cout << std::setw(2) << q << " " << std::fixed << std::setprecision(2)
      << r.avgWaiting << " " << r.avgTurnaround << " " << r.avgResponse
      << " " << std::setw(3) << r.contextSwitches << " "
      << std::string(r.contextSwitches, '#') << "\n";
    }
  }

    // ===== Case 6: Convoy effect =====
  std::cout << "\nCase 6. Convoy effect: FCFS vs MLFQ (makeConvoySet)\n";

  std::cout << "\n--- FCFS ---\n";
  {
    auto set = makeConvoySet();
    FcfsScheduler s(set);
    SimResult r = runSimulation(s);
    printResult(r);
    printProcessTable(s.processes());
  }

  std::cout << "\n--- SRTN ---\n";
  {
    auto set = makeConvoySet();
    SrtnScheduler s(set);
    SimResult r = runSimulation(s);
    printResult(r);
    printProcessTable(s.processes());
  }

  std::cout << "\n--- RR (q=4) ---\n";
  {
    auto set = makeConvoySet();
    RrScheduler s(set, 4);
    SimResult r = runSimulation(s);
    printResult(r);
    printProcessTable(s.processes());
  }

  std::cout << "\n--- Priority (preemptive) + aging ---\n";
  {
    auto set = makeConvoySet();
    PriorityScheduler s(set, true, true);
    SimResult r = runSimulation(s);
    printResult(r);
    printProcessTable(s.processes());
  }

  std::cout << "\n--- MLFQ ---\n";
  {
    auto set = makeConvoySet();
    MlfqScheduler s(set);
    SimResult r = runSimulation(s);
    printResult(r);
    printProcessTable(s.processes());
  }

  runExperiment10(0); // wait
  runExperiment10(1); // turn
  runExperiment10(2); // resp
  runExperiment10(3); // CPU%

  std::cout << "\nCase 7. Multicore (makeTestSet)\n";
  {
    auto set = makeTestSet();
    for (int cores : {1, 2, 4}) {
      FcfsScheduler s(set);
      printResult(runSimulationMulti(s, cores));
    }
    for (int cores : {1, 2, 4}) {
      SjfScheduler s(set);
      printResult(runSimulationMulti(s, cores));
    }
    for (int cores : {1, 2, 4}) {
      SrtnScheduler s(set);
      printResult(runSimulationMulti(s, cores));
    }
  }

  std::cout << "\nCase 8. EDF\n";
  {
    auto set = makeEdfTestSet();
    EdfScheduler edf(set);
    SimResult r = runSimulation(edf);
    printResult(r);
    printGantt(r);
    printProcessTable(edf.processes());

    // Проверка дедлайнов
    std::cout << "Deadline check:\n";
    for (auto& p : edf.processes()) {
      bool ok = p.finishTime <= p.deadline;
      std::cout << "  " << p.name
                << " finish=" << p.finishTime
                << " deadline=" << p.deadline
                << (ok ? "  OK" : "  MISS") << "\n";
    }
  }

  return 0;
}