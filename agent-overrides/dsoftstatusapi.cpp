#include "dsoftstatusapi.h"

#include "dsoftruntime.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
std::string compact(const QJsonObject &object) {
  return QJsonDocument(object).toJson(QJsonDocument::Compact).toStdString();
}
}

std::string DSoftStatusApi::healthJson() {
  QJsonObject root;
  root.insert(QStringLiteral("ok"), true);
  root.insert(QStringLiteral("service"),
              QStringLiteral("DSoft POS Printer Agent"));
  root.insert(QStringLiteral("queued_jobs"),
              DSoftRuntime::instance().queuedJobCount());
  root.insert(QStringLiteral("default_printer_code"),
              DSoftRuntime::instance()
                  .printerService()
                  .profileManager()
                  ->defaultProfileCode());
  return compact(root);
}

std::string DSoftStatusApi::printersJson() {
  auto &runtime = DSoftRuntime::instance();
  auto *manager = runtime.printerService().profileManager();
  const QString defaultCode = manager->defaultProfileCode();

  QJsonArray printers;
  for (const auto &profile : manager->profiles()) {
    QString readinessError;
    const bool ready = profile.enabled &&
                       runtime.dispatcher().printerReady(
                           profile.windowsPrinterName, &readinessError);

    QJsonObject value = profile.toJson();
    value.insert(QStringLiteral("default"), profile.code == defaultCode);
    value.insert(QStringLiteral("ready"), ready);
    value.insert(QStringLiteral("status"),
                 !profile.enabled
                     ? QStringLiteral("disabled")
                     : (ready ? QStringLiteral("ready")
                              : QStringLiteral("unavailable")));
    if (!readinessError.isEmpty())
      value.insert(QStringLiteral("status_message"), readinessError);
    printers.append(value);
  }

  QJsonObject root;
  root.insert(QStringLiteral("ok"), true);
  root.insert(QStringLiteral("queued_jobs"), runtime.queuedJobCount());
  root.insert(QStringLiteral("default_printer_code"), defaultCode);
  root.insert(QStringLiteral("printers"), printers);
  return compact(root);
}
