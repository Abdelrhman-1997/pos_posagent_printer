#include "dsoftlegacymigrator.h"

#include "printerprofilemanager.h"

#include <QSettings>

namespace {
QString firstExistingString(QSettings &settings, const QStringList &keys) {
  for (const auto &key : keys) {
    if (!settings.contains(key))
      continue;
    const QString value = settings.value(key).toString().trimmed();
    if (!value.isEmpty())
      return value;
  }
  return QString();
}

bool firstExistingBool(QSettings &settings, const QStringList &keys,
                       bool fallback = false) {
  for (const auto &key : keys) {
    if (settings.contains(key))
      return settings.value(key).toBool();
  }
  return fallback;
}
}

bool DSoftLegacyMigrator::migrateIfNeeded(PrinterProfileManager *manager,
                                          QString *message) {
  if (!manager) {
    if (message)
      *message = QStringLiteral("Printer profile manager is unavailable.");
    return false;
  }

  if (!manager->profiles().isEmpty())
    return true;

  QSettings legacy(QStringLiteral("PosAgentPRO"),
                   QStringLiteral("PosAgentPRO"));

  const QString printerName = firstExistingString(
      legacy,
      {QStringLiteral("printer_Windows Printer_field_name"),
       QStringLiteral("printer_Windows Printer_field_Name"),
       QStringLiteral("printer_name"),
       QStringLiteral("windows_printer_name")});

  if (printerName.isEmpty()) {
    if (message)
      *message = QStringLiteral("No legacy Windows printer setting was found.");
    return true;
  }

  const bool cashDrawer = firstExistingBool(
      legacy,
      {QStringLiteral("printer_Windows Printer_field_cash_drawer"),
       QStringLiteral("printer_Windows Printer_field_cashdrawer"),
       QStringLiteral("cash_drawer_enabled")},
      false);

  QString migrationError;
  const bool migrated = manager->migrateLegacySinglePrinter(
      printerName, cashDrawer, &migrationError);
  if (message) {
    *message = migrated
                   ? QStringLiteral("Legacy printer migrated to profile 'default'.")
                   : migrationError;
  }
  return migrated;
}
