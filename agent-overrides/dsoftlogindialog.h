#ifndef DSOFT_LOGIN_DIALOG_H
#define DSOFT_LOGIN_DIALOG_H

#include <QDialog>

class QLineEdit;
class QLabel;

class DSoftLoginDialog : public QDialog {
  Q_OBJECT

public:
  explicit DSoftLoginDialog(QWidget *parent = nullptr);

private slots:
  void attemptLogin();

private:
  QLineEdit *usernameEdit_ = nullptr;
  QLineEdit *passwordEdit_ = nullptr;
  QLabel *errorLabel_ = nullptr;
};

#endif // DSOFT_LOGIN_DIALOG_H
