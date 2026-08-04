#ifndef DSOFT_PRINTERPROFILEDIALOG_H
#define DSOFT_PRINTERPROFILEDIALOG_H

#include "printerprofile.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;

class PrinterProfileDialog : public QDialog {
  Q_OBJECT

public:
  explicit PrinterProfileDialog(const QStringList &installedPrinters,
                                QWidget *parent = nullptr);

  void setProfile(const PrinterProfile &profile);
  PrinterProfile profile() const;

private slots:
  void validateAndAccept();

private:
  QLineEdit *codeEdit_ = nullptr;
  QLineEdit *displayNameEdit_ = nullptr;
  QComboBox *windowsPrinterCombo_ = nullptr;
  QComboBox *roleCombo_ = nullptr;
  QSpinBox *paperWidthSpin_ = nullptr;
  QCheckBox *enabledCheck_ = nullptr;
  QCheckBox *cashDrawerCheck_ = nullptr;
};

#endif // DSOFT_PRINTERPROFILEDIALOG_H
