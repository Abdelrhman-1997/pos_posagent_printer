#include "dsoftlogindialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
const QString kAdminUsername = QStringLiteral("admin");
const QString kAdminPassword = QStringLiteral("admin");
}

DSoftLoginDialog::DSoftLoginDialog(QWidget *parent) : QDialog(parent) {
  setWindowTitle(QStringLiteral("DSoft POS Printer Agent Login"));
  setModal(true);
  setMinimumWidth(360);

  auto *title = new QLabel(QStringLiteral("Administrator Login"), this);
  QFont titleFont = title->font();
  titleFont.setBold(true);
  titleFont.setPointSize(titleFont.pointSize() + 2);
  title->setFont(titleFont);

  usernameEdit_ = new QLineEdit(this);
  usernameEdit_->setPlaceholderText(QStringLiteral("Username"));
  usernameEdit_->setText(QStringLiteral("admin"));
  usernameEdit_->selectAll();

  passwordEdit_ = new QLineEdit(this);
  passwordEdit_->setPlaceholderText(QStringLiteral("Password"));
  passwordEdit_->setEchoMode(QLineEdit::Password);

  errorLabel_ = new QLabel(this);
  errorLabel_->setStyleSheet(QStringLiteral("color: #b00020;"));
  errorLabel_->setVisible(false);

  auto *form = new QFormLayout;
  form->addRow(QStringLiteral("Username:"), usernameEdit_);
  form->addRow(QStringLiteral("Password:"), passwordEdit_);

  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok |
                                           QDialogButtonBox::Cancel,
                                       this);
  buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Login"));
  buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Exit"));

  auto *layout = new QVBoxLayout(this);
  layout->addWidget(title);
  layout->addLayout(form);
  layout->addWidget(errorLabel_);
  layout->addWidget(buttons);

  connect(buttons, &QDialogButtonBox::accepted, this,
          &DSoftLoginDialog::attemptLogin);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(passwordEdit_, &QLineEdit::returnPressed, this,
          &DSoftLoginDialog::attemptLogin);

  passwordEdit_->setFocus();
}

void DSoftLoginDialog::attemptLogin() {
  if (usernameEdit_->text() == kAdminUsername &&
      passwordEdit_->text() == kAdminPassword) {
    accept();
    return;
  }

  errorLabel_->setText(QStringLiteral("Invalid username or password."));
  errorLabel_->setVisible(true);
  passwordEdit_->clear();
  passwordEdit_->setFocus();
}
