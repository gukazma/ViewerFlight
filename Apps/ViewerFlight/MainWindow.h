#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFutureWatcher>
class QProgressDialog;
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent_ = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent* event_) override;

    void dropEvent(QDropEvent* event_) override;

    void addLayer(const QString& dir_);

    void createProgressDialog();

private:
    Ui::MainWindow *ui;
    QProgressDialog* m_prgDialog = nullptr;
    QFutureWatcher<void>   m_futureWather;
};

#endif // MAINWINDOW_H
