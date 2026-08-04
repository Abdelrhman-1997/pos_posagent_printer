#ifndef DSOFT_LOGGER_H
#define DSOFT_LOGGER_H

#include <QMutex>
#include <QString>
#include <QStringList>

class DSoftLogger {
public:
  static DSoftLogger &instance();

  void info(const QString &message);
  void warning(const QString &message);
  void error(const QString &message);

  QString logFilePath() const;
  QStringList recentLines(int maxLines = 200) const;
  void trimIfNeeded();

private:
  DSoftLogger();
  void write(const QString &level, const QString &message);

  mutable QMutex mutex_;
  QString logFilePath_;
};

#endif // DSOFT_LOGGER_H
