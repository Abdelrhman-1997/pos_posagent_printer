#include "dsoftstartupmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>

namespace {
constexpr auto kRunRegistryPath =
    "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr auto kRunValueName = "DSoft POS Printer Agent";
}

QString DSoftStartupManager::startupShortcutPath() {
  return QDir(QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation))
      .filePath(QStringLiteral("Startup/DSoft POS Printer Agent.lnk"));
}

bool DSoftStartupManager::setStartWithWindows(bool enabled,
                                              QString *errorMessage) {
#ifdef Q_OS_WIN
  QSettings runSettings(QStringLiteral(kRunRegistryPath),
                        QSettings::NativeFormat);
  if (enabled) {
    const QString executable = QDir::toNativeSeparators(
        QCoreApplication::applicationFilePath());
    if (executable.isEmpty()) {
      if (errorMessage)
        *errorMessage = QStringLiteral("Application executable path is empty.");
      return false;
    }
    runSettings.setValue(QStringLiteral(kRunValueName),
                         QStringLiteral("\"%1\" --minimized").arg(executable));
  } else {
    runSettings.remove(QStringLiteral(kRunValueName));
  }
  runSettings.sync();
  if (runSettings.status() != QSettings::NoError) {
    if (errorMessage)
      *errorMessage = QStringLiteral("Could not update Windows startup settings.");
    return false;
  }
  return true;
#else
  Q_UNUSED(enabled)
  if (errorMessage)
    *errorMessage = QStringLiteral("Start with Windows is only available on Windows.");
  return false;
#endif
}

bool DSoftStartupManager::startWithWindowsEnabled() {
#ifdef Q_OS_WIN
  QSettings runSettings(QStringLiteral(kRunRegistryPath),
                        QSettings::NativeFormat);
  return runSettings.contains(QStringLiteral(kRunValueName));
#else
  return false;
#endif
}
