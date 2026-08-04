#include "printerprofile.h"

#include <QRegularExpression>

bool PrinterProfile::isValid() const {
  static const QRegularExpression codePattern(QStringLiteral("^[a-z0-9][a-z0-9_-]{1,63}$"));
  return codePattern.match(code).hasMatch() && !displayName.trimmed().isEmpty() &&
         !windowsPrinterName.trimmed().isEmpty() && paperWidthMm > 0;
}

QJsonObject PrinterProfile::toJson() const {
  QJsonObject value;
  value.insert(QStringLiteral("code"), code);
  value.insert(QStringLiteral("display_name"), displayName);
  value.insert(QStringLiteral("windows_printer_name"), windowsPrinterName);
  value.insert(QStringLiteral("role"), role);
  value.insert(QStringLiteral("paper_width_mm"), paperWidthMm);
  value.insert(QStringLiteral("enabled"), enabled);
  value.insert(QStringLiteral("cash_drawer_enabled"), cashDrawerEnabled);
  return value;
}

PrinterProfile PrinterProfile::fromJson(const QJsonObject &value) {
  PrinterProfile profile;
  profile.code = value.value(QStringLiteral("code")).toString();
  profile.displayName = value.value(QStringLiteral("display_name")).toString();
  profile.windowsPrinterName =
      value.value(QStringLiteral("windows_printer_name")).toString();
  profile.role = value.value(QStringLiteral("role")).toString(QStringLiteral("receipt"));
  profile.paperWidthMm = value.value(QStringLiteral("paper_width_mm")).toInt(80);
  profile.enabled = value.value(QStringLiteral("enabled")).toBool(true);
  profile.cashDrawerEnabled =
      value.value(QStringLiteral("cash_drawer_enabled")).toBool(false);
  return profile;
}
