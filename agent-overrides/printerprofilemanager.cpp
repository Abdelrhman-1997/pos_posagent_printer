#include "printerprofilemanager.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QReadLocker>
#include <QSet>
#include <QWriteLocker>

namespace {
constexpr auto kProfilesKey = "dsoft/printer_profiles_json";
constexpr auto kDefaultProfileKey = "dsoft/default_printer_code";
}

PrinterProfileManager::PrinterProfileManager(QSettings *settings)
    : settings_(settings) {}

QList<PrinterProfile> PrinterProfileManager::profiles() const {
  QReadLocker locker(&lock_);
  return loadProfilesUnlocked();
}

bool PrinterProfileManager::profileByCode(const QString &code,
                                          PrinterProfile *profile) const {
  if (!profile)
    return false;

  QReadLocker locker(&lock_);
  const auto storedProfiles = loadProfilesUnlocked();
  for (const auto &stored : storedProfiles) {
    if (stored.code == code) {
      *profile = stored;
      return true;
    }
  }
  return false;
}

QString PrinterProfileManager::defaultProfileCode() const {
  QReadLocker locker(&lock_);
  return settings_->value(QStringLiteral(kDefaultProfileKey),
                          QStringLiteral("default"))
      .toString();
}

bool PrinterProfileManager::saveProfile(const PrinterProfile &profile,
                                        QString *errorMessage) {
  if (!profile.isValid()) {
    if (errorMessage)
      *errorMessage = QStringLiteral("Printer profile is incomplete or its code is invalid.");
    return false;
  }

  QWriteLocker locker(&lock_);
  auto storedProfiles = loadProfilesUnlocked();
  bool replaced = false;
  for (auto &stored : storedProfiles) {
    if (stored.code == profile.code) {
      stored = profile;
      replaced = true;
      break;
    }
  }
  if (!replaced)
    storedProfiles.append(profile);

  return writeProfilesUnlocked(storedProfiles, errorMessage);
}

bool PrinterProfileManager::removeProfile(const QString &code,
                                          QString *errorMessage) {
  QWriteLocker locker(&lock_);
  auto storedProfiles = loadProfilesUnlocked();
  const auto before = storedProfiles.size();
  storedProfiles.erase(
      std::remove_if(storedProfiles.begin(), storedProfiles.end(),
                     [&code](const PrinterProfile &profile) {
                       return profile.code == code;
                     }),
      storedProfiles.end());

  if (storedProfiles.size() == before) {
    if (errorMessage)
      *errorMessage = QStringLiteral("Printer profile was not found.");
    return false;
  }

  if (settings_->value(QStringLiteral(kDefaultProfileKey)).toString() == code) {
    settings_->setValue(QStringLiteral(kDefaultProfileKey),
                        storedProfiles.isEmpty() ? QString()
                                                 : storedProfiles.first().code);
  }
  return writeProfilesUnlocked(storedProfiles, errorMessage);
}

bool PrinterProfileManager::setDefaultProfileCode(const QString &code,
                                                  QString *errorMessage) {
  QWriteLocker locker(&lock_);
  const auto storedProfiles = loadProfilesUnlocked();
  const bool exists = std::any_of(
      storedProfiles.cbegin(), storedProfiles.cend(),
      [&code](const PrinterProfile &profile) { return profile.code == code; });
  if (!exists) {
    if (errorMessage)
      *errorMessage = QStringLiteral("Default printer profile does not exist.");
    return false;
  }
  settings_->setValue(QStringLiteral(kDefaultProfileKey), code);
  settings_->sync();
  return settings_->status() == QSettings::NoError;
}

bool PrinterProfileManager::migrateLegacySinglePrinter(
    const QString &windowsPrinterName, bool cashDrawerEnabled,
    QString *errorMessage) {
  if (windowsPrinterName.trimmed().isEmpty())
    return true;

  QWriteLocker locker(&lock_);
  auto storedProfiles = loadProfilesUnlocked();
  if (!storedProfiles.isEmpty())
    return true;

  PrinterProfile profile;
  profile.code = QStringLiteral("default");
  profile.displayName = QStringLiteral("Default Printer");
  profile.windowsPrinterName = windowsPrinterName;
  profile.role = QStringLiteral("receipt");
  profile.paperWidthMm = 80;
  profile.enabled = true;
  profile.cashDrawerEnabled = cashDrawerEnabled;
  storedProfiles.append(profile);

  if (!writeProfilesUnlocked(storedProfiles, errorMessage))
    return false;

  settings_->setValue(QStringLiteral(kDefaultProfileKey), profile.code);
  settings_->sync();
  return settings_->status() == QSettings::NoError;
}

QList<PrinterProfile> PrinterProfileManager::loadProfilesUnlocked() const {
  QList<PrinterProfile> profiles;
  const QByteArray raw = settings_->value(QStringLiteral(kProfilesKey)).toByteArray();
  if (raw.isEmpty())
    return profiles;

  QJsonParseError parseError;
  const auto document = QJsonDocument::fromJson(raw, &parseError);
  if (parseError.error != QJsonParseError::NoError || !document.isArray())
    return profiles;

  QSet<QString> seenCodes;
  for (const auto &item : document.array()) {
    if (!item.isObject())
      continue;
    const auto profile = PrinterProfile::fromJson(item.toObject());
    if (!profile.isValid() || seenCodes.contains(profile.code))
      continue;
    profiles.append(profile);
    seenCodes.insert(profile.code);
  }
  return profiles;
}

bool PrinterProfileManager::writeProfilesUnlocked(
    const QList<PrinterProfile> &profiles, QString *errorMessage) {
  QJsonArray array;
  for (const auto &profile : profiles)
    array.append(profile.toJson());

  settings_->setValue(QStringLiteral(kProfilesKey),
                      QJsonDocument(array).toJson(QJsonDocument::Compact));
  settings_->sync();
  if (settings_->status() != QSettings::NoError) {
    if (errorMessage)
      *errorMessage = QStringLiteral("Could not save printer profiles.");
    return false;
  }
  return true;
}
