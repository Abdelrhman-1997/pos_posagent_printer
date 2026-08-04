#include "printerprofiledialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSpinBox>
#include <QVBoxLayout>

PrinterProfileDialog::PrinterProfileDialog(
    const QStringList &installedPrinters, QWidget *parent)
    : QDialog(parent) {
  setWindowTitle(QStringLiteral("Printer Profile"));
  setModal(true);
  resize(460, 320);

  codeEdit_ = new QLineEdit(this);
  codeEdit_->setPlaceholderText(QStringLiteral("cashier_1"));

  displayNameEdit_ = new QLineEdit(this);
  displayNameEdit_->setPlaceholderText(QStringLiteral("Cashier Printer"));

  windowsPrinterCombo_ = new QComboBox(this);
  windowsPrinterCombo_->setEditable(true);
  windowsPrinterCombo_->addItems(installedPrinters);

  roleCombo_ = new QComboBox(this);
  roleCombo_->addItem(QStringLiteral("Receipt"), QStringLiteral("receipt"));
  roleCombo_->addItem(QStringLiteral("Preparation"),
                      QStringLiteral("preparation"));

  paperWidthSpin_ = new QSpinBox(this);
  paperWidthSpin_->setRange(40, 120);
  paperWidthSpin_->setValue(80);
  paperWidthSpin_->setSuffix(QStringLiteral(" mm"));

  enabledCheck_ = new QCheckBox(QStringLiteral("Enabled"), this);
  enabledCheck_->setChecked(true);

  cashDrawerCheck_ =
      new QCheckBox(QStringLiteral("Enable Cash Drawer"), this);

  auto *form = new QFormLayout();
  form->addRow(QStringLiteral("Printer Code"), codeEdit_);
  form->addRow(QStringLiteral("Display Name"), displayNameEdit_);
  form->addRow(QStringLiteral("Windows Printer"), windowsPrinterCombo_);
  form->addRow(QStringLiteral("Role"), roleCombo_);
  form->addRow(QStringLiteral("Paper Width"), paperWidthSpin_);
  form->addRow(QString(), enabledCheck_);
  form->addRow(QString(), cashDrawerCheck_);

  auto *buttons = new QDialogButtonBox(
      QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
  connect(buttons, &QDialogButtonBox::accepted, this,
          &PrinterProfileDialog::validateAndAccept);
  connect(buttons, &QDialogButtonBox::rejected, this,
          &QDialog::reject);

  auto *layout = new QVBoxLayout(this);
  layout->addLayout(form);
  layout->addStretch();
  layout->addWidget(buttons);
}

void PrinterProfileDialog::setProfile(const PrinterProfile &profile) {
  codeEdit_->setText(profile.code);
  displayNameEdit_->setText(profile.displayName);
  windowsPrinterCombo_->setCurrentText(profile.windowsPrinterName);

  const int roleIndex = roleCombo_->findData(profile.role);
  roleCombo_->setCurrentIndex(roleIndex >= 0 ? roleIndex : 0);

  paperWidthSpin_->setValue(profile.paperWidthMm > 0 ? profile.paperWidthMm
                                                     : 80);
  enabledCheck_->setChecked(profile.enabled);
  cashDrawerCheck_->setChecked(profile.cashDrawerEnabled);
}

PrinterProfile PrinterProfileDialog::profile() const {
  PrinterProfile value;
  value.code = codeEdit_->text().trimmed().toLower();
  value.displayName = displayNameEdit_->text().trimmed();
  value.windowsPrinterName = windowsPrinterCombo_->currentText().trimmed();
  value.role = roleCombo_->currentData().toString();
  value.paperWidthMm = paperWidthSpin_->value();
  value.enabled = enabledCheck_->isChecked();
  value.cashDrawerEnabled = cashDrawerCheck_->isChecked();
  return value;
}

void PrinterProfileDialog::validateAndAccept() {
  static const QRegularExpression codePattern(
      QStringLiteral("^[a-z0-9][a-z0-9_-]{0,63}$"));

  const PrinterProfile value = profile();
  if (!codePattern.match(value.code).hasMatch()) {
    QMessageBox::warning(
        this, QStringLiteral("Invalid Printer Code"),
        QStringLiteral("Use lowercase letters, numbers, underscore or dash. "
                       "Example: kitchen_1"));
    codeEdit_->setFocus();
    return;
  }

  if (value.displayName.isEmpty()) {
    QMessageBox::warning(this, QStringLiteral("Missing Display Name"),
                         QStringLiteral("Enter a display name for the printer."));
    displayNameEdit_->setFocus();
    return;
  }

  if (value.windowsPrinterName.isEmpty()) {
    QMessageBox::warning(
        this, QStringLiteral("Missing Windows Printer"),
        QStringLiteral("Select or enter the Windows printer name."));
    windowsPrinterCombo_->setFocus();
    return;
  }

  accept();
}
