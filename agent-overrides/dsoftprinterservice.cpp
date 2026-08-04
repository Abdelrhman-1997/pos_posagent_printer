#include "dsoftprinterservice.h"

#include <QMutexLocker>
#include <QSettings>

namespace {
constexpr int kMaxRecentResults = 100;
}

DSoftPrinterService::DSoftPrinterService(QSettings *settings)
    : profileManager_(settings) {}

PrinterProfileManager *DSoftPrinterService::profileManager() {
  return &profileManager_;
}

const PrinterProfileManager *DSoftPrinterService::profileManager() const {
  return &profileManager_;
}

QString DSoftPrinterService::resolvePrinterCode(
    const QString &requestedCode) const {
  const QString trimmed = requestedCode.trimmed();
  return trimmed.isEmpty() ? profileManager_.defaultProfileCode() : trimmed;
}

quint64 DSoftPrinterService::enqueueReceipt(const QString &printerCode,
                                            QByteArray jpegData) {
  std::uint64_t sequence = 0;
  QString error;
  if (!queue_.enqueue(resolvePrinterCode(printerCode),
                      DSoftPrintJobType::Receipt, std::move(jpegData),
                      &sequence, &error)) {
    DSoftJobResult result;
    result.printerCode = resolvePrinterCode(printerCode);
    result.success = false;
    result.message = error;
    rememberResult(result);
    return 0;
  }
  return static_cast<quint64>(sequence);
}

quint64 DSoftPrinterService::enqueueCashDrawer(const QString &printerCode) {
  std::uint64_t sequence = 0;
  QString error;
  if (!queue_.enqueue(resolvePrinterCode(printerCode),
                      DSoftPrintJobType::CashDrawer, QByteArray(), &sequence,
                      &error)) {
    DSoftJobResult result;
    result.printerCode = resolvePrinterCode(printerCode);
    result.success = false;
    result.message = error;
    rememberResult(result);
    return 0;
  }
  return static_cast<quint64>(sequence);
}

bool DSoftPrinterService::processNext(const DispatchCallback &dispatch,
                                      DSoftJobResult *result) {
  const auto next = queue_.takeNext();
  if (!next.has_value())
    return false;

  DSoftJobResult current;
  current.sequence = static_cast<quint64>(next->sequence);
  current.printerCode = next->printerCode;

  PrinterProfile profile;
  if (!profileManager_.profileByCode(next->printerCode, &profile)) {
    current.message = QStringLiteral("Printer profile was not found.");
  } else if (!profile.enabled) {
    current.message = QStringLiteral("Printer profile is disabled.");
  } else if (!dispatch) {
    current.message = QStringLiteral("Printer dispatch callback is missing.");
  } else {
    QString dispatchError;
    current.success = dispatch(profile, *next, &dispatchError);
    current.message = current.success
                          ? QStringLiteral("Print job submitted successfully.")
                          : (dispatchError.isEmpty()
                                 ? QStringLiteral("Print job submission failed.")
                                 : dispatchError);
  }

  rememberResult(current);
  if (result)
    *result = current;
  return true;
}

int DSoftPrinterService::queuedJobCount() const { return queue_.size(); }

QList<DSoftJobResult> DSoftPrinterService::recentResults() const {
  QMutexLocker locker(&resultsMutex_);
  return recentResults_;
}

void DSoftPrinterService::rememberResult(const DSoftJobResult &result) {
  QMutexLocker locker(&resultsMutex_);
  recentResults_.prepend(result);
  while (recentResults_.size() > kMaxRecentResults)
    recentResults_.removeLast();
}
