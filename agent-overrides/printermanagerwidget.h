#ifndef DSOFT_PRINTERMANAGERWIDGET_H
#define DSOFT_PRINTERMANAGERWIDGET_H

#include <QWidget>

class QLabel;
class QPushButton;
class QTableWidget;

class PrinterManagerWidget : public QWidget {
  Q_OBJECT

public:
  explicit PrinterManagerWidget(QWidget *parent = nullptr);
  void refresh();

signals:
  void profilesChanged();

private slots:
  void addProfile();
  void editProfile();
  void removeProfile();
  void setDefaultProfile();
  void testSelectedProfile();

private:
  QString selectedPrinterCode() const;
  void updateSummary();

  QLabel *summaryLabel_ = nullptr;
  QTableWidget *table_ = nullptr;
  QPushButton *editButton_ = nullptr;
  QPushButton *removeButton_ = nullptr;
  QPushButton *defaultButton_ = nullptr;
  QPushButton *testButton_ = nullptr;
};

#endif // DSOFT_PRINTERMANAGERWIDGET_H
