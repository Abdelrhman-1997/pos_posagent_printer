#ifndef DSOFT_STARTUPMANAGER_H
#define DSOFT_STARTUPMANAGER_H

#include <QString>

class DSoftStartupManager {
public:
  static bool setStartWithWindows(bool enabled, QString *errorMessage = nullptr);
  static bool startWithWindowsEnabled();
  static QString startupShortcutPath();
};

#endif // DSOFT_STARTUPMANAGER_H
