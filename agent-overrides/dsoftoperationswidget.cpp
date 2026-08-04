#include "dsoftoperationswidget.h"

#include "dsoftlogger.h"
#include "dsoftruntime.h"
#include "dsoftstartupmanager.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

DSoftOperationsWidget::DSoftOperationsWidget(QWidget *parent) : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);

  auto *topRow = new QHBoxLayout();
  startupCheck_ = new QCheckBox(QStringLiteral("Start with Windows"), this);
  startupCheck_->setChecked(DSoftStartupManager::startWithWindowsEnabled());
  queueLabel_ = new QLabel(this);
  auto *refreshButton = new QPushButton(QStringLiteral("Refresh"), this);
  auto *openLogsButton = new QPushButton(QStringLiteral("Open Logs Folder"), this);
  topRow->addWidget(startupCheck_);
  topRow->addStretch();
  topRow->addWidget(queueLabel_);
  topRow->addWidget(refreshButton);
  topRow->addWidget(openLogsButton);
  layout->addLayout(topRow);

  jobsTable_ = new QTableWidget(this);
  jobsTable_->setColumnCount(4);
  jobsTable_->setHorizontalHeaderLabels(
      {QStringLiteral("Job"), QStringLiteral("Printer"),
       QStringLiteral("Result"), QStringLiteral("Message")});
  jobsTable_->horizontalHeader()->setStretchLastSection(true);
  jobsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  jobsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
  layout->addWidget(new QLabel(QStringLiteral("Recent Jobs"), this));
  layout->addWidget(jobsTable_);

  logPathLabel_ = new QLabel(this);
  logPathLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  logsEdit_ = new QTextEdit(this);
  logsEdit_->setReadOnly(true);
  logsEdit_->setLineWrapMode(QTextEdit::NoWrap);
  layout->addWidget(new QLabel(QStringLiteral("Application Log"), this));
  layout->addWidget(logPathLabel_);
  layout->addWidget(logsEdit_);

  connect(startupCheck_, &QCheckBox::toggled, this,
          &DSoftOperationsWidget::toggleStartup);
  connect(refreshButton, &QPushButton::clicked, this,
          &DSoftOperationsWidget::refresh);
  connect(openLogsButton, &QPushButton::clicked, this,
          &DSoftOperationsWidget::openLogFolder);

  refreshTimer_ = new QTimer(this);
  refreshTimer_->setInterval(2000);
  connect(refreshTimer_, &QTimer::timeout, this, &DSoftOperationsWidget::refresh);
  refreshTimer_->start();
  refresh();
}

void DSoftOperationsWidget::refresh() {
  queueLabel_->setText(QStringLiteral("Queued jobs: %1")
                           .arg(DSoftRuntime::instance().queuedJobCount()));
  refreshJobs();
  refreshLogs();
}

void DSoftOperationsWidget::toggleStartup(bool enabled) {
  QString error;
  if (DSoftStartupManager::setStartWithWindows(enabled, &error))
    return;

  startupCheck_->blockSignals(true);
  startupCheck_->setChecked(!enabled);
  startupCheck_->blockSignals(false);
  QMessageBox::warning(this, QStringLiteral("Start with Windows"), error);
}

void DSoftOperationsWidget::openLogFolder() {
  const QFileInfo info(DSoftLogger::instance().logFilePath());
  QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()));
}

void DSoftOperationsWidget::refreshJobs() {
  const auto results = DSoftRuntime::instance().printerService().recentResults();
  jobsTable_->setRowCount(results.size());
  for (int row = 0; row < results.size(); ++row) {
    const auto &result = results.at(row);
    jobsTable_->setItem(row, 0,
                        new QTableWidgetItem(QString::number(result.sequence)));
    jobsTable_->setItem(row, 1, new QTableWidgetItem(result.printerCode));
    jobsTable_->setItem(
        row, 2,
        new QTableWidgetItem(result.success ? QStringLiteral("Success")
                                            : QStringLiteral("Failed")));
    jobsTable_->setItem(row, 3, new QTableWidgetItem(result.message));
  }
  jobsTable_->resizeColumnsToContents();
}

void DSoftOperationsWidget::refreshLogs() {
  const QString path = DSoftLogger::instance().logFilePath();
  logPathLabel_->setText(path);
  logsEdit_->setPlainText(DSoftLogger::instance().recentLines(200).join('\n'));
  auto cursor = logsEdit_->textCursor();
  cursor.movePosition(QTextCursor::End);
  logsEdit_->setTextCursor(cursor);
}
