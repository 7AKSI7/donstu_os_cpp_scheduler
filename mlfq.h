#pragma once
#include <deque>
#include <vector>
#include <algorithm>
#include <limits>
#include "scheduler.h"

// Multi-Level Feedback Queue
// Уровни: q=2, q=4, q=∞ (последний — фактически FCFS)
class MlfqScheduler : public Scheduler {
public:
  explicit MlfqScheduler(std::vector<Process> processes)
    : Scheduler(std::move(processes)) {
    queues_.resize(3);
    quantums_ = {2, 4, std::numeric_limits<std::uint64_t>::max()};
    levelQuantumUsed_.assign(3, 0);
  }

  std::string name() const override { return "MLFQ"; }

  void onTick(std::uint64_t tick) override {
    for (auto& p : processes_) {
      if (p.state == ProcessState::NEW && p.arrivalTime <= tick) {
        p.state = ProcessState::READY;
        p.queueLevel = 0;
        queues_[0].push_back(p.pid);
      }
    }
  }

  void onProcessReady(int pid, std::uint64_t) override {
    Process* p = find(pid);
    if (!p) return;
    // Процесс вернулся из I/O — оставляем его на текущем уровне (feedback)
    queues_[p->queueLevel].push_back(pid);
  }

  int pickNext(std::uint64_t) override {
    for (int lvl = 0; lvl < static_cast<int>(queues_.size()); ++lvl) {
      if (!queues_[lvl].empty()) {
        currentLevel_ = lvl;
        levelQuantumUsed_[lvl] = 0;
        return queues_[lvl].front();
      }
    }
    return -1;
  }

  void onProcessFinished(int pid, std::uint64_t) override {
    Process* p = find(pid);
    if (p) p->state = ProcessState::TERMINATED;
    for (auto& q : queues_) {
      auto it = std::find(q.begin(), q.end(), pid);
      if (it != q.end()) { q.erase(it); return; }
    }
  }

  void onProcessBlocked(int pid, std::uint64_t) override {
    for (auto& q : queues_) {
      auto it = std::find(q.begin(), q.end(), pid);
      if (it != q.end()) { q.erase(it); return; }
    }
  }

  void onProcessPreempted(int /*pid*/, std::uint64_t) override {
    // Перемещение по уровням сделано в shouldPreempt
  }

  bool shouldPreempt(int currentPid, std::uint64_t) override {
    // Более приоритетная очередь не пуста
    for (int lvl = 0; lvl < currentLevel_; ++lvl) {
      if (!queues_[lvl].empty()) return true;
    }
    // Квант текущего уровня истёк
    if (static_cast<std::uint64_t>(levelQuantumUsed_[currentLevel_]) >= quantums_[currentLevel_]) {
      Process* p = find(currentPid);
      if (p && !p->isFinished()) {
        // Удаляем из текущей очереди
        auto& cur = queues_[currentLevel_];
        auto it = std::find(cur.begin(), cur.end(), currentPid);
        if (it != cur.end()) cur.erase(it);
        // Опускаем на уровень ниже
        int newLevel = std::min(currentLevel_ + 1, static_cast<int>(queues_.size()) - 1);
        p->queueLevel = newLevel;
        queues_[newLevel].push_back(currentPid);
      }
      return true;
    }
    return false;
  }

  void onProcessRanTick(int /*pid*/) override {
    levelQuantumUsed_[currentLevel_]++;
  }

private:
  std::vector<std::deque<int>> queues_;
  std::vector<std::uint64_t> quantums_;
  std::vector<int> levelQuantumUsed_;
  int currentLevel_ = 0;
};
