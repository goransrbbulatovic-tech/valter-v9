#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include "theme.h"
#include "lang.h"

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    // Šta je korisnik odabrao (čita mainwindow NAKON exec())
    Theme::ThemeType selectedTheme() const { return (Theme::ThemeType)cbTheme->currentIndex(); }
    Lang::Language   selectedLang()  const { return cbLang->currentIndex()==1 ? Lang::EN : Lang::SR; }

private slots:

private:
    QComboBox   *cbTheme;
    QComboBox   *cbLang;
    QPushButton *btnApply;
    QPushButton *btnClose;
};

#endif
