#include "printerroutingrequest.h"

#include <QJsonObject>
#include <QRegularExpression>

namespace {
bool isValidPrinterCode(const QString &code) {
  static const QRegularExpression pattern(
      QStringLiteral("^[a-z0-9][a-z0-9_-]{0,63}$"));
  return pattern.match(code).hasMatch();
}
}

PrinterRoutingRequest PrinterRoutingRequest::fromDataObject(
    const QJsonObject &data, const QString &defaultPrinterCode) {
  PrinterRoutingRequest request;
  request.action = data.value(QStringLiteral("action")).toString().trimmed();

  // Backward compatibility: existing Odoo clients do not send printer_code.
  request.printerCode =
      data.value(QStringLiteral("printer_code")).toString().trimmed();
  if (request.printerCode.isEmpty())
    request.printerCode = defaultPrinterCode.trimmed();

  if (!isValidPrinterCode(request.printerCode)) {
    request.error = QStringLiteral("Invalid or missing printer_code.");
    return request;
  }

  if (request.action == QStringLiteral("print_receipt")) {
    request.receiptBase64 =
        data.value(QStringLiteral("receipt")).toString().trimmed();
    if (request.receiptBase64.isEmpty()) {
      request.error = QStringLiteral("Receipt payload is empty.");
      return request;
    }
  } else if (request.action != QStringLiteral("cashbox")) {
    request.error = QStringLiteral("Unsupported printer action.");
    return request;
  }

  request.valid = true;
  return request;
}
