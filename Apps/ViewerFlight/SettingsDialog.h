#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QWidget>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H
