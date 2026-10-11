#pragma once
#include <deque>
#include <algorithm>
#include "scheduler.h"

// First-Come, First-Served
class FcfsScheduler : public Scheduler {
public:
  using Scheduler::Scheduler;

  std::string name() const override { return "FCFS"; }

  void onTick(std::uint64_t tick) override {
    for (auto& p : processes_) {
      if (p.state == ProcessState::NEW && p.arrivalTime <= tick) {
        p.state = ProcessState::READY;
        queue_.push_back(p.pid);
      }
    }
  }

  void onProcessReady(int pid, std::uint64_t) override {
    // вернулся из I/O — в конец очереди
    queue_.push_back(pid);
  }

  int pickNext(std::uint64_t) override {
    if (queue_.empty()) return -1;
    int pid = queue_.front();
    queue_.pop_front();
    return pid;
  }


  void onProcessFinished(int pid, std::uint64_t) override {
    Process* p = find(pid);
    if (p) p->state = ProcessState::TERMINATED;
    auto it = std::find(queue_.begin(), queue_.end(), pid);
    if (it != queue_.end()) queue_.erase(it);
  }

  void onProcessBlocked(int pid, std::uint64_t) override {
    auto it = std::find(queue_.begin(), queue_.end(), pid);
    if (it != queue_.end()) queue_.erase(it);
  }

  bool shouldPreempt(int, std::uint64_t) override { return false; }

private:
  std::deque<int> queue_;
};
