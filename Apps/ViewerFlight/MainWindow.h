#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFutureWatcher>
#include <set>
#include <boost/filesystem/path.hpp>
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
    std::set<boost::filesystem::path> m_tileLoaded;
};

#endif // MAINWINDOW_H
