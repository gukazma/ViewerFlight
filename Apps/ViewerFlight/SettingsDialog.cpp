#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"
#include "UAVMVS/Settings.h"
#include "UAVMVS/Context.hpp"

#include <memory>
SettingsDialog::SettingsDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    connect(ui->pushButton_OK, &QPushButton::clicked, [=] { 
        this->updateSettings();
        this->close();
    });

    connect(ui->pushButton_Cancel, &QPushButton::clicked, [=] {
        this->close();
    });
    
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::updateSettings() {
    std::shared_ptr<::uavmvs::context::Settings> settings = std::make_shared<::uavmvs::context::Settings>();
    settings->diskRadius = ui->spinBox_radius->value();
    settings->focal = ui->doubleSpinBox_focal->value();
    ::uavmvs::context::AttachSettings(settings);
}
