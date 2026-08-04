#ifndef DSOFT_SINGLEINSTANCE_H
#define DSOFT_SINGLEINSTANCE_H

#include <QLockFile>
#include <QString>

#include <memory>

class DSoftSingleInstance {
public:
  explicit DSoftSingleInstance(const QString &applicationId);

  bool tryAcquire(QString *errorMessage = nullptr);
  bool isPrimaryInstance() const;

private:
  QString lockFilePath() const;

  QString applicationId_;
  std::unique_ptr<QLockFile> lockFile_;
  bool primary_ = false;
};

#endif // DSOFT_SINGLEINSTANCE_H
