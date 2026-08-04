#include "printermanagerwidget.h"

#include "dsoftruntime.h"
#include "printerprofiledialog.h"

#include <QBuffer>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {
QByteArray buildTestReceipt() {
  QImage image(576, 420, QImage::Format_RGB32);
  image.fill(Qt::white);

  QPainter painter(&image);
  painter.setPen(Qt::black);
  QFont titleFont = painter.font();
  titleFont.setPointSize(22);
  titleFont.setBold(true);
  painter.setFont(titleFont);
  painter.drawText(QRect(0, 35, image.width(), 50), Qt::AlignCenter,
                   QStringLiteral("DSoft POS Printer Agent"));

  QFont bodyFont = painter.font();
  bodyFont.setPointSize(15);
  bodyFont.setBold(false);
  painter.setFont(bodyFont);
  painter.drawText(QRect(30, 115, image.width() - 60, 40), Qt::AlignCenter,
                   QStringLiteral("Printer test completed successfully"));
  painter.drawText(QRect(30, 170, image.width() - 60, 40), Qt::AlignCenter,
                   QStringLiteral("Odoo Local Printing Service"));
  painter.drawLine(40, 245, image.width() - 40, 245);
  painter.drawText(QRect(30, 275, image.width() - 60, 40), Qt::AlignCenter,
                   QStringLiteral("www.dsoftx.tech"));
  painter.end();

  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "JPEG", 92);
  return bytes;
}
}

PrinterManagerWidget::PrinterManagerWidget(QWidget *parent) : QWidget(parent) {
  auto *title = new QLabel(QStringLiteral("Printers"), this);
  QFont titleFont = title->font();
  titleFont.setPointSize(20);
  titleFont.setBold(true);
  title->setFont(titleFont);

  summaryLabel_ = new QLabel(this);

  table_ = new QTableWidget(this);
  table_->setColumnCount(7);
  table_->setHorizontalHeaderLabels(
      {QStringLiteral("Code"), QStringLiteral("Name"),
       QStringLiteral("Windows Printer"), QStringLiteral("Role"),
       QStringLiteral("Width"), QStringLiteral("Status"),
       QStringLiteral("Default")});
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setSelectionMode(QAbstractItemView::SingleSelection);
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
  table_->verticalHeader()->setVisible(false);

  auto *addButton = new QPushButton(QStringLiteral("Add Printer"), this);
  editButton_ = new QPushButton(QStringLiteral("Edit"), this);
  removeButton_ = new QPushButton(QStringLiteral("Remove"), this);
  defaultButton_ = new QPushButton(QStringLiteral("Set Default"), this);
  testButton_ = new QPushButton(QStringLiteral("Test Print"), this);

  auto *buttons = new QHBoxLayout();
  buttons->addWidget(addButton);
  buttons->addWidget(editButton_);
  buttons->addWidget(removeButton_);
  buttons->addWidget(defaultButton_);
  buttons->addStretch();
  buttons->addWidget(testButton_);

  auto *layout = new QVBoxLayout(this);
  layout->addWidget(title);
  layout->addWidget(summaryLabel_);
  layout->addLayout(buttons);
  layout->addWidget(table_);

  connect(addButton, &QPushButton::clicked, this,
          &PrinterManagerWidget::addProfile);
  connect(editButton_, &QPushButton::clicked, this,
          &PrinterManagerWidget::editProfile);
  connect(removeButton_, &QPushButton::clicked, this,
          &PrinterManagerWidget::removeProfile);
  connect(defaultButton_, &QPushButton::clicked, this,
          &PrinterManagerWidget::setDefaultProfile);
  connect(testButton_, &QPushButton::clicked, this,
          &PrinterManagerWidget::testSelectedProfile);
  connect(table_, &QTableWidget::itemDoubleClicked, this,
          [this](QTableWidgetItem *) { editProfile(); });

  refresh();
}

void PrinterManagerWidget::refresh() {
  auto &runtime = DSoftRuntime::instance();
  const auto profiles = runtime.printerService().profileManager()->profiles();
  const QString defaultCode =
      runtime.printerService().profileManager()->defaultProfileCode();

  table_->setRowCount(profiles.size());
  for (int row = 0; row < profiles.size(); ++row) {
    const auto &profile = profiles.at(row);
    QString statusError;
    const bool ready = runtime.dispatcher().printerReady(
        profile.windowsPrinterName, &statusError);

    const QStringList values = {
        profile.code,
        profile.displayName,
        profile.windowsPrinterName,
        profile.role == QStringLiteral("preparation")
            ? QStringLiteral("Preparation")
            : QStringLiteral("Receipt"),
        QStringLiteral("%1 mm").arg(profile.paperWidthMm),
        !profile.enabled ? QStringLiteral("Disabled")
                         : (ready ? QStringLiteral("Ready")
                                  : QStringLiteral("Unavailable")),
        profile.code == defaultCode ? QStringLiteral("Yes") : QString()};

    for (int column = 0; column < values.size(); ++column) {
      auto *item = new QTableWidgetItem(values.at(column));
      item->setData(Qt::UserRole, profile.code);
      if (column == 5 && !ready)
        item->setToolTip(statusError);
      table_->setItem(row, column, item);
    }
  }
  updateSummary();
}

QString PrinterManagerWidget::selectedPrinterCode() const {
  const int row = table_->currentRow();
  if (row < 0 || !table_->item(row, 0))
    return QString();
  return table_->item(row, 0)->data(Qt::UserRole).toString();
}

void PrinterManagerWidget::updateSummary() {
  const auto profiles = DSoftRuntime::instance()
                            .printerService()
                            .profileManager()
                            ->profiles();
  int enabled = 0;
  for (const auto &profile : profiles) {
    if (profile.enabled)
      ++enabled;
  }
  summaryLabel_->setText(
      QStringLiteral("%1 configured printer(s) • %2 enabled • %3 queued job(s)")
          .arg(profiles.size())
          .arg(enabled)
          .arg(DSoftRuntime::instance().queuedJobCount()));
}

void PrinterManagerWidget::addProfile() {
  PrinterProfileDialog dialog(
      DSoftRuntime::instance().dispatcher().installedPrinters(), this);
  if (dialog.exec() != QDialog::Accepted)
    return;

  QString error;
  if (!DSoftRuntime::instance()
           .printerService()
           .profileManager()
           ->saveProfile(dialog.profile(), &error)) {
    QMessageBox::critical(this, QStringLiteral("Could Not Save Printer"),
                          error);
    return;
  }
  refresh();
  emit profilesChanged();
}

void PrinterManagerWidget::editProfile() {
  const QString code = selectedPrinterCode();
  if (code.isEmpty())
    return;

  PrinterProfile existing;
  auto *manager =
      DSoftRuntime::instance().printerService().profileManager();
  if (!manager->profileByCode(code, &existing))
    return;

  PrinterProfileDialog dialog(
      DSoftRuntime::instance().dispatcher().installedPrinters(), this);
  dialog.setProfile(existing);
  if (dialog.exec() != QDialog::Accepted)
    return;

  const PrinterProfile updated = dialog.profile();
  QString error;
  if (updated.code != existing.code) {
    PrinterProfile duplicate;
    if (manager->profileByCode(updated.code, &duplicate)) {
      QMessageBox::warning(this, QStringLiteral("Duplicate Printer Code"),
                           QStringLiteral("That printer code already exists."));
      return;
    }
    if (!manager->removeProfile(existing.code, &error)) {
      QMessageBox::critical(this, QStringLiteral("Could Not Update Printer"),
                            error);
      return;
    }
  }

  if (!manager->saveProfile(updated, &error)) {
    QMessageBox::critical(this, QStringLiteral("Could Not Update Printer"),
                          error);
    return;
  }
  refresh();
  emit profilesChanged();
}

void PrinterManagerWidget::removeProfile() {
  const QString code = selectedPrinterCode();
  if (code.isEmpty())
    return;

  if (QMessageBox::question(
          this, QStringLiteral("Remove Printer"),
          QStringLiteral("Remove printer profile '%1'?").arg(code)) !=
      QMessageBox::Yes)
    return;

  QString error;
  if (!DSoftRuntime::instance()
           .printerService()
           .profileManager()
           ->removeProfile(code, &error)) {
    QMessageBox::critical(this, QStringLiteral("Could Not Remove Printer"),
                          error);
    return;
  }
  refresh();
  emit profilesChanged();
}

void PrinterManagerWidget::setDefaultProfile() {
  const QString code = selectedPrinterCode();
  if (code.isEmpty())
    return;

  QString error;
  if (!DSoftRuntime::instance()
           .printerService()
           .profileManager()
           ->setDefaultProfileCode(code, &error)) {
    QMessageBox::critical(this,
                          QStringLiteral("Could Not Set Default Printer"),
                          error);
    return;
  }
  refresh();
  emit profilesChanged();
}

void PrinterManagerWidget::testSelectedProfile() {
  const QString code = selectedPrinterCode();
  if (code.isEmpty()) {
    QMessageBox::information(this, QStringLiteral("Select Printer"),
                             QStringLiteral("Select a printer first."));
    return;
  }

  const quint64 sequence = DSoftRuntime::instance()
                               .printerService()
                               .enqueueReceipt(code, buildTestReceipt());
  if (!sequence) {
    QMessageBox::critical(this, QStringLiteral("Test Print Failed"),
                          QStringLiteral("The test job could not be queued."));
    return;
  }

  DSoftJobResult result;
  DSoftRuntime::instance().processNext(&result);
  if (result.success) {
    QMessageBox::information(
        this, QStringLiteral("Test Print"),
        QStringLiteral("Test job #%1 was sent successfully.").arg(sequence));
  } else {
    QMessageBox::critical(this, QStringLiteral("Test Print Failed"),
                          result.message);
  }
  refresh();
}
