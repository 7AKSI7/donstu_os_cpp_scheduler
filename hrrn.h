#pragma once
#include "scheduler.h"
#include <algorithm>
#include <vector>

// Highest Response Ratio Next (невытесняющий)
// Приоритет: R = (waitingTime + burstTime) / burstTime
class HrrnScheduler : public Scheduler {
public:
  using Scheduler::Scheduler;

  std::string name() const override { return "HRRN"; }

  void onTick(std::uint64_t tick) override {
    for (auto& p : processes_) {
      if (p.state == ProcessState::NEW && p.arrivalTime <= tick) {
        p.state = ProcessState::READY;
        ready_.push_back(p.pid);
      }
    }
  }

  void onProcessReady(int pid, std::uint64_t) override {
    ready_.push_back(pid);
  }

  int pickNext(std::uint64_t) override {
    if (ready_.empty()) return -1;

    auto ratio = [&](int pid) -> double {
      const Process* p = find(pid);
      // R = (waitingTime + burstTime) / burstTime
      // double обязателен, иначе целочисленное деление
      return static_cast<double>(p->waitingTime + p->burstTime)
           / static_cast<double>(p->burstTime);
    };

    auto it = std::max_element(ready_.begin(), ready_.end(),
      [&](int a, int b) { return ratio(a) < ratio(b); });
    int pid = *it;
    ready_.erase(it);
    return pid;
  }

  void onProcessFinished(int pid, std::uint64_t) override {
    Process* p = find(pid);
    if (p) p->state = ProcessState::TERMINATED;
  }

  void onProcessBlocked(int /*pid*/, std::uint64_t) override {
    // Невытесняющий: если процесс ушёл в I/O, он вернётся через onProcessReady
  }

  void onProcessPreempted(int pid, std::uint64_t) override {
    ready_.push_back(pid);  // на всякий случай
  }

  bool shouldPreempt(int, std::uint64_t) override { return false; }

private:
  std::vector<int> ready_;
};