#ifndef DSOFT_PRINTERPROFILE_H
#define DSOFT_PRINTERPROFILE_H

#include <QJsonObject>
#include <QString>

struct PrinterProfile {
  QString code;
  QString displayName;
  QString windowsPrinterName;
  QString role;
  int paperWidthMm = 80;
  bool enabled = true;
  bool cashDrawerEnabled = false;

  bool isValid() const;
  QJsonObject toJson() const;
  static PrinterProfile fromJson(const QJsonObject &value);
};

#endif // DSOFT_PRINTERPROFILE_H
