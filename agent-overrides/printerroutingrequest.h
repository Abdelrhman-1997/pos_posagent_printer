#ifndef DSOFT_PRINTERROUTINGREQUEST_H
#define DSOFT_PRINTERROUTINGREQUEST_H

#include <QString>

class QJsonObject;

struct PrinterRoutingRequest {
  QString printerCode;
  QString action;
  QString receiptBase64;
  bool valid = false;
  QString error;

  static PrinterRoutingRequest fromDataObject(const QJsonObject &data,
                                              const QString &defaultPrinterCode);
};

#endif // DSOFT_PRINTERROUTINGREQUEST_H
