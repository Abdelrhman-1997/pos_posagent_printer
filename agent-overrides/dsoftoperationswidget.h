#ifndef DSOFT_OPERATIONSWIDGET_H
#define DSOFT_OPERATIONSWIDGET_H

#include <QWidget>

class QCheckBox;
class QLabel;
class QPushButton;
class QTableWidget;
class QTextEdit;
class QTimer;

class DSoftOperationsWidget : public QWidget {
  Q_OBJECT

public:
  explicit DSoftOperationsWidget(QWidget *parent = nullptr);

private slots:
  void refresh();
  void toggleStartup(bool enabled);
  void openLogFolder();

private:
  void refreshJobs();
  void refreshLogs();

  QCheckBox *startupCheck_ = nullptr;
  QLabel *queueLabel_ = nullptr;
  QLabel *logPathLabel_ = nullptr;
  QTableWidget *jobsTable_ = nullptr;
  QTextEdit *logsEdit_ = nullptr;
  QTimer *refreshTimer_ = nullptr;
};

#endif // DSOFT_OPERATIONSWIDGET_H
