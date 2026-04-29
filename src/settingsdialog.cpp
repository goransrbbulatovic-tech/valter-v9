#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFrame>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(TR("Podešavanja", "Settings"));
    setFixedSize(440, 380);
    setModal(true);

    QVBoxLayout *main = new QVBoxLayout(this);
    main->setSpacing(14);
    main->setContentsMargins(24, 20, 24, 18);

    QLabel *lblTitle = new QLabel(TR("⚙  Podešavanja programa", "⚙  Program Settings"));
    lblTitle->setStyleSheet("font-size:16px; font-weight:800; color:#60a5fa;");
    main->addWidget(lblTitle);

    QFrame *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("background:#2563eb; border:none; max-height:2px;");
    main->addWidget(sep);

    // Tema
    QGroupBox *grpTheme = new QGroupBox(TR("Izgled / Tema", "Appearance / Theme"));
    QVBoxLayout *thL = new QVBoxLayout(grpTheme);
    cbTheme = new QComboBox();
    cbTheme->addItem("🌑  " + TR("Tamna", "Dark"));
    cbTheme->addItem("☀️  " + TR("Svijetla", "Light"));
    cbTheme->addItem("🎬  Cinema");
    cbTheme->addItem("🔷  Blue Steel");
    cbTheme->setCurrentIndex((int)Theme::instance().current());
    cbTheme->setMinimumHeight(36);
    thL->addWidget(cbTheme);
    thL->addWidget(new QLabel(TR("Promjena se primijeni klikom na Primijeni.",
                                  "Change is applied by clicking Apply.")));
    main->addWidget(grpTheme);

    // Jezik
    QGroupBox *grpLang = new QGroupBox(TR("Jezik / Language", "Language / Jezik"));
    QVBoxLayout *lgL = new QVBoxLayout(grpLang);
    cbLang = new QComboBox();
    cbLang->addItem("🇧🇦  Srpski");
    cbLang->addItem("🇬🇧  English");
    cbLang->setCurrentIndex(Lang::instance().isEN() ? 1 : 0);
    cbLang->setMinimumHeight(36);
    lgL->addWidget(cbLang);
    lgL->addWidget(new QLabel(TR("Promjena se primijeni klikom na Primijeni.",
                                  "Change is applied by clicking Apply.")));
    main->addWidget(grpLang);

    main->addStretch();

    QFrame *sep2 = new QFrame();
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("background:#1e2535; border:none; max-height:1px;");
    main->addWidget(sep2);

    QHBoxLayout *btnL = new QHBoxLayout();
    btnClose = new QPushButton(TR("Zatvori", "Close"));
    btnApply = new QPushButton(TR("✔  Primijeni", "✔  Apply"));
    btnApply->setObjectName("btnDodaj");
    btnApply->setMinimumWidth(130);
    btnApply->setMinimumHeight(38);
    btnL->addWidget(btnClose);
    btnL->addStretch();
    btnL->addWidget(btnApply);
    main->addLayout(btnL);

    // SAMO accept() — mainwindow čita selectedTheme()/selectedLang() sam
    connect(btnApply, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);
}

