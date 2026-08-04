#ifndef DSOFT_PRINTQUEUE_H
#define DSOFT_PRINTQUEUE_H

#include "dsoftprintjob.h"

#include <QMutex>
#include <QQueue>
#include <QString>

#include <atomic>
#include <optional>

// One FIFO for all targets preserves order exactly as jobs are accepted.
// The printer code travels with each job, so processing never depends on a
// mutable global "currently selected printer" value.
class DSoftPrintQueue {
public:
  bool enqueue(const QString &printerCode, DSoftPrintJobType type,
               QByteArray payload, std::uint64_t *sequence = nullptr,
               QString *errorMessage = nullptr);

  std::optional<DSoftPrintJob> takeNext();
  int size() const;
  bool isEmpty() const;
  void clear();

private:
  mutable QMutex mutex_;
  QQueue<DSoftPrintJob> queue_;
  std::atomic<std::uint64_t> nextSequence_{1};
};

#endif // DSOFT_PRINTQUEUE_H
