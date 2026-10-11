#pragma once
#include "scheduler.h"
#include <algorithm>
#include <vector>

// Earliest Deadline First (вытесняющий)
// Выбирается процесс с наименьшим абсолютным дедлайном.
class EdfScheduler : public Scheduler {
public:
  using Scheduler::Scheduler;

  std::string name() const override { return "EDF"; }

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
    auto it = std::min_element(ready_.begin(), ready_.end(),
                               [&](int a, int b) {
                                 return find(a)->deadline < find(b)->deadline;
                               });
    int pid = *it;
    ready_.erase(it);
    return pid;
  }

  void onProcessFinished(int pid, std::uint64_t) override {
    Process* p = find(pid);
    if (p) p->state = ProcessState::TERMINATED;
  }

  void onProcessBlocked(int /*pid*/, std::uint64_t) override {
    // вернётся через onProcessReady
  }

  void onProcessPreempted(int pid, std::uint64_t) override {
    ready_.push_back(pid);
  }

  bool shouldPreempt(int currentPid, std::uint64_t) override {
    Process* cur = find(currentPid);
    if (!cur || cur->isFinished()) return false;
    for (int pid : ready_) {
      Process* p = find(pid);
      if (p && p->deadline < cur->deadline) return true;
    }
    return false;
  }

private:
  std::vector<int> ready_;
};