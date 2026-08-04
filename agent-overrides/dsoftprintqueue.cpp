#include "dsoftprintqueue.h"

#include <QMutexLocker>

bool DSoftPrintQueue::enqueue(const QString &printerCode,
                              DSoftPrintJobType type, QByteArray payload,
                              std::uint64_t *sequence,
                              QString *errorMessage) {
  DSoftPrintJob job;
  job.sequence = nextSequence_.fetch_add(1, std::memory_order_relaxed);
  job.printerCode = printerCode.trimmed();
  job.type = type;
  job.payload = std::move(payload);
  job.enqueuedAt = std::chrono::steady_clock::now();

  if (!job.isValid()) {
    if (errorMessage)
      *errorMessage = QStringLiteral("Print job is incomplete.");
    return false;
  }

  {
    QMutexLocker locker(&mutex_);
    queue_.enqueue(job);
  }

  if (sequence)
    *sequence = job.sequence;
  return true;
}

std::optional<DSoftPrintJob> DSoftPrintQueue::takeNext() {
  QMutexLocker locker(&mutex_);
  if (queue_.isEmpty())
    return std::nullopt;
  return queue_.dequeue();
}

int DSoftPrintQueue::size() const {
  QMutexLocker locker(&mutex_);
  return queue_.size();
}

bool DSoftPrintQueue::isEmpty() const {
  QMutexLocker locker(&mutex_);
  return queue_.isEmpty();
}

void DSoftPrintQueue::clear() {
  QMutexLocker locker(&mutex_);
  queue_.clear();
}
