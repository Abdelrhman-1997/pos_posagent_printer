#ifndef DSOFT_WINDOWSPRINTERDISPATCHER_H
#define DSOFT_WINDOWSPRINTERDISPATCHER_H

#include "dsoftprintjob.h"
#include "printerprofile.h"

#include <QMutex>
#include <QString>
#include <QStringList>

#include <memory>

class PrinterWindowsSpooler;

class DSoftWindowsPrinterDispatcher {
public:
  DSoftWindowsPrinterDispatcher();
  ~DSoftWindowsPrinterDispatcher();

  bool dispatch(const PrinterProfile &profile, const DSoftPrintJob &job,
                QString *errorMessage = nullptr);
  QStringList installedPrinters() const;
  bool printerExists(const QString &windowsPrinterName) const;
  bool printerReady(const QString &windowsPrinterName,
                    QString *errorMessage = nullptr) const;

private:
  bool dispatchReceipt(const PrinterProfile &profile,
                       const DSoftPrintJob &job, QString *errorMessage);
  bool dispatchCashDrawer(const PrinterProfile &profile,
                          QString *errorMessage);

  mutable QMutex mutex_;
  std::unique_ptr<PrinterWindowsSpooler> spooler_;
};

#endif // DSOFT_WINDOWSPRINTERDISPATCHER_H
