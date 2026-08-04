#ifndef DSOFT_PRINTERSERVICE_H
#define DSOFT_PRINTERSERVICE_H

#include "dsoftprintqueue.h"
#include "printerprofilemanager.h"

#include <QList>
#include <QMutex>
#include <QString>

#include <functional>

class QSettings;

struct DSoftJobResult {
  quint64 sequence = 0;
  QString printerCode;
  bool success = false;
  QString message;
};

class DSoftPrinterService {
public:
  using DispatchCallback =
      std::function<bool(const PrinterProfile &, const DSoftPrintJob &, QString *)>;

  explicit DSoftPrinterService(QSettings *settings);

  PrinterProfileManager *profileManager();
  const PrinterProfileManager *profileManager() const;

  quint64 enqueueReceipt(const QString &printerCode, QByteArray jpegData);
  quint64 enqueueCashDrawer(const QString &printerCode);

  bool processNext(const DispatchCallback &dispatch, DSoftJobResult *result);
  int queuedJobCount() const;
  QList<DSoftJobResult> recentResults() const;

private:
  QString resolvePrinterCode(const QString &requestedCode) const;
  void rememberResult(const DSoftJobResult &result);

  PrinterProfileManager profileManager_;
  DSoftPrintQueue queue_;
  mutable QMutex resultsMutex_;
  QList<DSoftJobResult> recentResults_;
};

#endif // DSOFT_PRINTERSERVICE_H
