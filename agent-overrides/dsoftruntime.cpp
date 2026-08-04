#include "dsoftruntime.h"

DSoftRuntime &DSoftRuntime::instance() {
  static DSoftRuntime runtime;
  return runtime;
}

DSoftRuntime::DSoftRuntime()
    : settings_(QStringLiteral("DSoft"),
                QStringLiteral("DSoft POS Printer Agent")),
      printerService_(&settings_) {}

DSoftPrinterService &DSoftRuntime::printerService() {
  return printerService_;
}

DSoftWindowsPrinterDispatcher &DSoftRuntime::dispatcher() {
  return dispatcher_;
}

bool DSoftRuntime::processNext(DSoftJobResult *result) {
  return printerService_.processNext(
      [this](const PrinterProfile &profile, const DSoftPrintJob &job,
             QString *errorMessage) {
        return dispatcher_.dispatch(profile, job, errorMessage);
      },
      result);
}

int DSoftRuntime::queuedJobCount() const {
  return printerService_.queuedJobCount();
}
