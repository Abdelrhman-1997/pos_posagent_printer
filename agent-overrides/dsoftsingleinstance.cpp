#include "dsoftsingleinstance.h"

#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>

DSoftSingleInstance::DSoftSingleInstance(const QString &applicationId)
    : applicationId_(applicationId.trimmed()) {}

QString DSoftSingleInstance::lockFilePath() const {
  QString base = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
  if (base.isEmpty())
    base = QDir::tempPath();
  QDir().mkpath(base);
  QString safeId = applicationId_;
  safeId.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_.-]")),
                 QStringLiteral("_"));
  return QDir(base).filePath(safeId + QStringLiteral(".lock"));
}

bool DSoftSingleInstance::tryAcquire(QString *errorMessage) {
  lockFile_ = std::make_unique<QLockFile>(lockFilePath());
  lockFile_->setStaleLockTime(30000);
  if (lockFile_->tryLock(0)) {
    primary_ = true;
    return true;
  }

  primary_ = false;
  if (errorMessage)
    *errorMessage = QStringLiteral("DSoft POS Printer Agent is already running.");
  return false;
}

bool DSoftSingleInstance::isPrimaryInstance() const { return primary_; }
