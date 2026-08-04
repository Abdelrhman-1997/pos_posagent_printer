#ifndef DSOFT_PRINTJOB_H
#define DSOFT_PRINTJOB_H

#include <QByteArray>
#include <QString>

#include <chrono>
#include <cstdint>

// Immutable data required to route one local print operation safely.
enum class DSoftPrintJobType {
  Receipt,
  CashDrawer,
};

struct DSoftPrintJob {
  std::uint64_t sequence = 0;
  QString printerCode;
  DSoftPrintJobType type = DSoftPrintJobType::Receipt;
  QByteArray payload;
  std::chrono::steady_clock::time_point enqueuedAt;

  bool isValid() const {
    if (printerCode.trimmed().isEmpty())
      return false;
    return type == DSoftPrintJobType::CashDrawer || !payload.isEmpty();
  }
};

#endif // DSOFT_PRINTJOB_H
