#ifndef DSOFT_PRINTERPROFILEMANAGER_H
#define DSOFT_PRINTERPROFILEMANAGER_H

#include "printerprofile.h"

#include <QList>
#include <QReadWriteLock>
#include <QSettings>
#include <QString>

class PrinterProfileManager {
public:
  explicit PrinterProfileManager(QSettings *settings);

  QList<PrinterProfile> profiles() const;
  bool profileByCode(const QString &code, PrinterProfile *profile) const;
  QString defaultProfileCode() const;

  bool saveProfile(const PrinterProfile &profile, QString *errorMessage = nullptr);
  bool removeProfile(const QString &code, QString *errorMessage = nullptr);
  bool setDefaultProfileCode(const QString &code, QString *errorMessage = nullptr);

  // Converts the existing single-printer installation into one compatible
  // profile. Existing customers keep printing without manual reconfiguration.
  bool migrateLegacySinglePrinter(const QString &windowsPrinterName,
                                  bool cashDrawerEnabled,
                                  QString *errorMessage = nullptr);

private:
  QList<PrinterProfile> loadProfilesUnlocked() const;
  bool writeProfilesUnlocked(const QList<PrinterProfile> &profiles,
                             QString *errorMessage);

  QSettings *settings_;
  mutable QReadWriteLock lock_;
};

#endif // DSOFT_PRINTERPROFILEMANAGER_H
