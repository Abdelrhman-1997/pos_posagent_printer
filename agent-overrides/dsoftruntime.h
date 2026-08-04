#ifndef DSOFT_RUNTIME_H
#define DSOFT_RUNTIME_H

#include "dsoftprinterservice.h"
#include "dsoftwindowsprinterdispatcher.h"

#include <QSettings>

class DSoftRuntime {
public:
  static DSoftRuntime &instance();

  DSoftPrinterService &printerService();
  DSoftWindowsPrinterDispatcher &dispatcher();

  bool processNext(DSoftJobResult *result = nullptr);
  int queuedJobCount() const;

private:
  DSoftRuntime();
  DSoftRuntime(const DSoftRuntime &) = delete;
  DSoftRuntime &operator=(const DSoftRuntime &) = delete;

  QSettings settings_;
  DSoftPrinterService printerService_;
  DSoftWindowsPrinterDispatcher dispatcher_;
};

#endif // DSOFT_RUNTIME_H
