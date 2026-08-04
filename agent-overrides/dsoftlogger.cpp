#include "dsoftlogger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>

namespace {
constexpr qint64 kMaxLogBytes = 2 * 1024 * 1024;
constexpr int kMaxRotatedFiles = 3;
}

DSoftLogger &DSoftLogger::instance() {
  static DSoftLogger logger;
  return logger;
}

DSoftLogger::DSoftLogger() {
  const QString baseDir = QStandardPaths::writableLocation(
      QStandardPaths::AppLocalDataLocation);
  QDir dir(baseDir);
  if (!dir.exists())
    dir.mkpath(QStringLiteral("."));
  logFilePath_ = dir.filePath(QStringLiteral("dsoft-pos-printer-agent.log"));
}

void DSoftLogger::info(const QString &message) {
  write(QStringLiteral("INFO"), message);
}

void DSoftLogger::warning(const QString &message) {
  write(QStringLiteral("WARN"), message);
}

void DSoftLogger::error(const QString &message) {
  write(QStringLiteral("ERROR"), message);
}

QString DSoftLogger::logFilePath() const { return logFilePath_; }

void DSoftLogger::write(const QString &level, const QString &message) {
  QMutexLocker locker(&mutex_);
  trimIfNeeded();

  QFile file(logFilePath_);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    return;

  QTextStream stream(&file);
  stream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " ["
         << level << "] " << message.trimmed() << '\n';
}

QStringList DSoftLogger::recentLines(int maxLines) const {
  QMutexLocker locker(&mutex_);
  QFile file(logFilePath_);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return {};

  QStringList lines;
  QTextStream stream(&file);
  while (!stream.atEnd()) {
    lines.append(stream.readLine());
    while (lines.size() > qMax(1, maxLines))
      lines.removeFirst();
  }
  return lines;
}

void DSoftLogger::trimIfNeeded() {
  QFile current(logFilePath_);
  if (!current.exists() || current.size() < kMaxLogBytes)
    return;

  for (int index = kMaxRotatedFiles - 1; index >= 1; --index) {
    const QString source = QStringLiteral("%1.%2").arg(logFilePath_).arg(index);
    const QString target =
        QStringLiteral("%1.%2").arg(logFilePath_).arg(index + 1);
    if (QFile::exists(target))
      QFile::remove(target);
    if (QFile::exists(source))
      QFile::rename(source, target);
  }

  const QString firstRotation = logFilePath_ + QStringLiteral(".1");
  if (QFile::exists(firstRotation))
    QFile::remove(firstRotation);
  current.rename(firstRotation);
}
