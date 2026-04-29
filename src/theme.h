#ifndef THEME_H
#define THEME_H
#include <QString>
#include <QApplication>
#include <QPalette>
#include <QSettings>
#include <QTimer>

class Theme {
public:
    enum ThemeType { DARK=0, LIGHT=1, CINEMA=2, BLUE=3 };

    static Theme& instance() { static Theme t; return t; }

    ThemeType current() const { return m_theme; }

    void load() {
        QSettings s("KCBar","Valter");
        int v = s.value("UI/Theme", (int)DARK).toInt();
        m_theme = (ThemeType)qBound(0, v, 3);
    }

    void set(ThemeType t, bool deferred = false) {
        m_theme = t;
        QSettings s("KCBar","Valter");
        s.setValue("UI/Theme",(int)t);
        if (deferred) applyDeferred();
        else apply();
    }

    QString name() const {
        switch(m_theme){
            case LIGHT:  return "Light";
            case CINEMA: return "Cinema";
            case BLUE:   return "Blue Steel";
            default:     return "Dark";
        }
    }

    // Boje pozadine reda po statusu
    QString rowBg(const QString &status, bool alt) const {
        if(m_theme==LIGHT)  return lightRowBg(status,alt);
        if(m_theme==CINEMA) return cinemaRowBg(status,alt);
        if(m_theme==BLUE)   return blueRowBg(status,alt);
        return darkRowBg(status,alt);
    }

    // Stylesheet
    QString stylesheet() const {
        switch(m_theme){
            case LIGHT:  return lightStyle();
            case CINEMA: return cinemaStyle();
            case BLUE:   return blueStyle();
            default:     return darkStyle();
        }
    }

    void applyDeferred() {
        // Defer to next event loop — prevents crash when called from dialog
        QTimer::singleShot(0, [this]() { apply(); });
    }

    void apply() {
        qApp->setStyle("Fusion");
        QPalette p;
        if(m_theme==LIGHT){
            p.setColor(QPalette::Window,          QColor(245,247,252));
            p.setColor(QPalette::WindowText,      QColor(15,23,42));
            p.setColor(QPalette::Base,            QColor(255,255,255));
            p.setColor(QPalette::AlternateBase,   QColor(241,245,249));
            p.setColor(QPalette::Text,            QColor(15,23,42));
            p.setColor(QPalette::Button,          QColor(226,232,240));
            p.setColor(QPalette::ButtonText,      QColor(15,23,42));
            p.setColor(QPalette::Highlight,       QColor(37,99,235));
            p.setColor(QPalette::HighlightedText, Qt::white);
            p.setColor(QPalette::Disabled,QPalette::ButtonText,QColor(148,163,184));
        } else if(m_theme==CINEMA){
            p.setColor(QPalette::Window,          QColor(20,10,5));
            p.setColor(QPalette::WindowText,      QColor(240,220,180));
            p.setColor(QPalette::Base,            QColor(12,6,2));
            p.setColor(QPalette::AlternateBase,   QColor(28,14,6));
            p.setColor(QPalette::Text,            QColor(240,220,180));
            p.setColor(QPalette::Button,          QColor(40,20,8));
            p.setColor(QPalette::ButtonText,      QColor(240,220,180));
            p.setColor(QPalette::Highlight,       QColor(180,130,20));
            p.setColor(QPalette::HighlightedText, QColor(10,6,2));
            p.setColor(QPalette::Disabled,QPalette::ButtonText,QColor(100,80,50));
        } else if(m_theme==BLUE){
            p.setColor(QPalette::Window,          QColor(8,16,38));
            p.setColor(QPalette::WindowText,      QColor(186,214,255));
            p.setColor(QPalette::Base,            QColor(4,10,26));
            p.setColor(QPalette::AlternateBase,   QColor(12,24,52));
            p.setColor(QPalette::Text,            QColor(186,214,255));
            p.setColor(QPalette::Button,          QColor(16,32,72));
            p.setColor(QPalette::ButtonText,      QColor(186,214,255));
            p.setColor(QPalette::Highlight,       QColor(0,112,243));
            p.setColor(QPalette::HighlightedText, Qt::white);
            p.setColor(QPalette::Disabled,QPalette::ButtonText,QColor(60,90,140));
        } else { // DARK
            p.setColor(QPalette::Window,          QColor(18,22,32));
            p.setColor(QPalette::WindowText,      QColor(220,228,240));
            p.setColor(QPalette::Base,            QColor(14,18,26));
            p.setColor(QPalette::AlternateBase,   QColor(24,28,40));
            p.setColor(QPalette::Text,            QColor(220,228,240));
            p.setColor(QPalette::Button,          QColor(36,42,58));
            p.setColor(QPalette::ButtonText,      QColor(220,228,240));
            p.setColor(QPalette::Highlight,       QColor(37,99,235));
            p.setColor(QPalette::HighlightedText, Qt::white);
            p.setColor(QPalette::Disabled,QPalette::ButtonText,QColor(80,92,115));
        }
        qApp->setPalette(p);
        qApp->setStyleSheet(stylesheet());
    }

public:
    ThemeType m_theme = DARK;  // settingsdialog accesses directly
private:
    Theme() { load(); }

    // ── DARK ──────────────────────────────────────────────────────────────
    QString darkRowBg(const QString &st, bool alt) const {
        if(st=="uredu")  return alt?"#0f2016":"#0c1a10";
        if(st=="upo")    return alt?"#261e06":"#1e1604";
        if(st=="krit")   return alt?"#2a1204":"#220e02";
        if(st=="ist")    return alt?"#280808":"#200404";
        if(st=="neo")    return alt?"#1e1030":"#180c28";
        return alt?"#1e2230":"#181c28";
    }
    QString darkStyle() const { return R"(
* { font-family:'Segoe UI',sans-serif; }
QMainWindow,QDialog { background:#12161e; }
QTabWidget::pane { border:1px solid #242b3d; background:#12161e; border-radius:0 6px 6px 6px; }
QTabBar::tab { background:#0e1118; color:#6b7fa8; padding:11px 28px; border:1px solid #1e2535;
  border-bottom:none; border-top-left-radius:7px; border-top-right-radius:7px;
  font-size:13px; font-weight:600; margin-right:3px; min-width:160px; }
QTabBar::tab:selected { background:#12161e; color:#60a5fa; border-bottom:3px solid #3b82f6; }
QTabBar::tab:hover:!selected { background:#181e2e; color:#93c5fd; }
QPushButton { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2d3650,stop:1 #222840);
  color:#c8d4f0; border:1px solid #3a4460; border-radius:7px; padding:9px 20px;
  font-size:13px; font-weight:600; min-width:100px; }
QPushButton:hover { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #3a4870,stop:1 #2a3458);
  border-color:#5a7acc; color:#e0eaff; }
QPushButton:pressed { background:#1e2845; }
QPushButton:disabled { color:#3a4460; border-color:#242b3d; background:#181e2e; }
QPushButton#btnDodaj { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2563eb,stop:1 #1d4ed8); border-color:#3b82f6; color:#fff; }
QPushButton#btnDodaj:hover { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #3b82f6,stop:1 #2563eb); }
QPushButton#btnObrisi,QPushButton#btnObrisiArh { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #991b1b,stop:1 #7f1d1d); border-color:#ef4444; color:#fecaca; }
QPushButton#btnArhiviraj { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1e40af,stop:1 #1e3a8a); border-color:#60a5fa; color:#bfdbfe; }
QPushButton#btnVratiArh { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #166534,stop:1 #14532d); border-color:#4ade80; color:#bbf7d0; }
QPushButton#btnPDF { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #7e22ce,stop:1 #6b21a8); border-color:#a855f7; color:#e9d5ff; }
QPushButton#btnCSV { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #065f46,stop:1 #064e3b); border-color:#34d399; color:#a7f3d0; }
QPushButton#btnUvezi { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #92400e,stop:1 #78350f); border-color:#fbbf24; color:#fef3c7; }
QTableWidget { background:#0e1118; gridline-color:transparent; border:1px solid #1e2535; border-radius:8px; font-size:13px; outline:none; }
QTableWidget::item { border:none; }
QTableWidget::item:selected { background:transparent; }
QHeaderView { background:#0a0d14; }
QHeaderView::section { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1a2035,stop:1 #131826);
  color:#7b92c4; padding:10px 8px; border:none; border-right:1px solid #1e2535;
  border-bottom:2px solid #2563eb; font-weight:700; font-size:11px; letter-spacing:0.8px; }
QScrollBar:vertical { background:#0e1118; width:12px; border-radius:6px; }
QScrollBar::handle:vertical { background:#2d3a55; border-radius:6px; min-height:30px; }
QScrollBar::handle:vertical:hover { background:#3b4e72; }
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
QStatusBar { background:#090c12; color:#4a5e85; font-size:12px; border-top:1px solid #1a2035; padding:3px 8px; }
QLineEdit { background:#141a28; border:1px solid #2d3a55; border-radius:7px; padding:8px 14px; color:#c8d4f0; font-size:13px; }
QLineEdit:focus { border-color:#3b82f6; background:#161d2e; }
QGroupBox { border:1px solid #2d3a55; border-radius:8px; margin-top:14px; font-weight:700; font-size:12px; color:#6b82aa; padding-top:6px; }
QGroupBox::title { subcontrol-origin:margin; left:14px; padding:0 6px; background:#12161e; }
QLabel#lblUpozorenje { color:#fbbf24; font-size:13px; font-weight:700; padding:7px 16px; background:#1c1507; border:1px solid #92400e; border-radius:7px; }
QLabel#lblKriticno { color:#fca5a5; font-size:13px; font-weight:700; padding:7px 16px; background:#1f0707; border:1px solid #7f1d1d; border-radius:7px; }
QMessageBox { background:#12161e; color:#c8d4f0; }
QComboBox { background:#141a28; border:1px solid #2d3a55; border-radius:6px; padding:6px 10px; color:#c8d4f0; }
QComboBox::drop-down { border:none; }
QComboBox QAbstractItemView { background:#141a28; color:#c8d4f0; selection-background-color:#1e40af; border:1px solid #2d3a55; }
QDateEdit { background:#141a28; border:1px solid #2d3a55; border-radius:4px; padding:5px 10px; color:#c8d4f0; }
QTextEdit { background:#141a28; border:1px solid #2d3a55; border-radius:4px; padding:6px 10px; color:#c8d4f0; }
QCheckBox { color:#dce1eb; }
QCheckBox::indicator { width:16px; height:16px; border:1px solid #3d4a62; border-radius:3px; background:#0f1318; }
QCheckBox::indicator:checked { background:#1d4ed8; border-color:#3b82f6; }
QSpinBox { background:#141a28; border:1px solid #2d3a55; border-radius:6px; padding:6px 10px; color:#c8d4f0; }
QListWidget { background:#141a28; border:1px solid #2d3a55; border-radius:8px; color:#60a5fa; }
QListWidget::item { padding:6px 12px; border-bottom:1px solid #1e2535; }
QListWidget::item:selected { background:#1e3a8a; color:white; border-radius:4px; }
)"; }

    // ── LIGHT ─────────────────────────────────────────────────────────────
    QString lightRowBg(const QString &st, bool alt) const {
        if(st=="uredu")  return alt?"#f0fdf4":"#f7fef9";
        if(st=="upo")    return alt?"#fffbeb":"#fefce8";
        if(st=="krit")   return alt?"#fff7ed":"#fefaf5";
        if(st=="ist")    return alt?"#fef2f2":"#fef8f8";
        if(st=="neo")    return alt?"#faf5ff":"#fdf8ff";
        return alt?"#f8fafc":"#ffffff";
    }
    QString lightStyle() const { return R"(
* { font-family:'Segoe UI',sans-serif; }
QMainWindow,QDialog { background:#f1f5fb; }
QTabWidget::pane { border:1px solid #cbd5e1; background:#f1f5fb; border-radius:0 6px 6px 6px; }
QTabBar::tab { background:#e2e8f4; color:#475569; padding:11px 28px; border:1px solid #cbd5e1;
  border-bottom:none; border-top-left-radius:7px; border-top-right-radius:7px;
  font-size:13px; font-weight:600; margin-right:3px; min-width:160px; }
QTabBar::tab:selected { background:#f1f5fb; color:#1d4ed8; border-bottom:3px solid #3b82f6; }
QTabBar::tab:hover:!selected { background:#dde4f0; }
QPushButton { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #e8eef8,stop:1 #d8e2f4);
  color:#1e3a6e; border:1px solid #93acd4; border-radius:7px; padding:9px 20px;
  font-size:13px; font-weight:600; min-width:100px; }
QPushButton:hover { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #dce8fa,stop:1 #c8d8f0);
  border-color:#3b82f6; }
QPushButton:pressed { background:#bfd4ef; }
QPushButton:disabled { color:#94a3b8; border-color:#cbd5e1; background:#e8eef8; }
QPushButton#btnDodaj { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #3b82f6,stop:1 #2563eb); border-color:#1d4ed8; color:#fff; }
QPushButton#btnObrisi,QPushButton#btnObrisiArh { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #ef4444,stop:1 #dc2626); border-color:#b91c1c; color:#fff; }
QPushButton#btnArhiviraj { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #60a5fa,stop:1 #3b82f6); border-color:#2563eb; color:#fff; }
QPushButton#btnVratiArh { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #22c55e,stop:1 #16a34a); border-color:#15803d; color:#fff; }
QPushButton#btnPDF { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #a855f7,stop:1 #9333ea); border-color:#7e22ce; color:#fff; }
QPushButton#btnCSV { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #10b981,stop:1 #059669); border-color:#047857; color:#fff; }
QPushButton#btnUvezi { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #f59e0b,stop:1 #d97706); border-color:#b45309; color:#fff; }
QTableWidget { background:#ffffff; gridline-color:transparent; border:1px solid #cbd5e1; border-radius:8px; font-size:13px; outline:none; color:#0f172a; }
QTableWidget::item { border:none; color:#0f172a; }
QTableWidget::item:selected { background:transparent; }
QHeaderView { background:#e2e8f4; }
QHeaderView::section { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #e8eef8,stop:1 #d8e2f4);
  color:#3b5a9a; padding:10px 8px; border:none; border-right:1px solid #cbd5e1;
  border-bottom:2px solid #3b82f6; font-weight:700; font-size:11px; letter-spacing:0.8px; }
QScrollBar:vertical { background:#e8eef8; width:12px; border-radius:6px; }
QScrollBar::handle:vertical { background:#93acd4; border-radius:6px; min-height:30px; }
QScrollBar::handle:vertical:hover { background:#3b82f6; }
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
QStatusBar { background:#e2e8f4; color:#475569; font-size:12px; border-top:1px solid #cbd5e1; padding:3px 8px; }
QLineEdit { background:#ffffff; border:1px solid #93acd4; border-radius:7px; padding:8px 14px; color:#0f172a; font-size:13px; }
QLineEdit:focus { border-color:#3b82f6; }
QGroupBox { border:1px solid #93acd4; border-radius:8px; margin-top:14px; font-weight:700; font-size:12px; color:#3b5a9a; padding-top:6px; }
QGroupBox::title { subcontrol-origin:margin; left:14px; padding:0 6px; background:#f1f5fb; }
QLabel#lblUpozorenje { color:#92400e; font-size:13px; font-weight:700; padding:7px 16px; background:#fef3c7; border:1px solid #f59e0b; border-radius:7px; }
QLabel#lblKriticno { color:#991b1b; font-size:13px; font-weight:700; padding:7px 16px; background:#fee2e2; border:1px solid #ef4444; border-radius:7px; }
QMessageBox { background:#f1f5fb; color:#0f172a; }
QComboBox { background:#ffffff; border:1px solid #93acd4; border-radius:6px; padding:6px 10px; color:#0f172a; }
QComboBox::drop-down { border:none; }
QComboBox QAbstractItemView { background:#ffffff; color:#0f172a; selection-background-color:#3b82f6; selection-color:#fff; border:1px solid #93acd4; }
QDateEdit { background:#ffffff; border:1px solid #93acd4; border-radius:4px; padding:5px 10px; color:#0f172a; }
QTextEdit { background:#ffffff; border:1px solid #93acd4; border-radius:4px; padding:6px 10px; color:#0f172a; }
QCheckBox { color:#0f172a; }
QCheckBox::indicator { width:16px; height:16px; border:1px solid #93acd4; border-radius:3px; background:#ffffff; }
QCheckBox::indicator:checked { background:#3b82f6; border-color:#1d4ed8; }
QSpinBox { background:#ffffff; border:1px solid #93acd4; border-radius:6px; padding:6px 10px; color:#0f172a; }
QListWidget { background:#ffffff; border:1px solid #93acd4; border-radius:8px; color:#1d4ed8; }
QListWidget::item { padding:6px 12px; border-bottom:1px solid #e2e8f4; color:#0f172a; }
QListWidget::item:selected { background:#3b82f6; color:white; border-radius:4px; }
)"; }

    // ── CINEMA (crveno-zlatna) ─────────────────────────────────────────────
    QString cinemaRowBg(const QString &st, bool alt) const {
        if(st=="uredu")  return alt?"#0d1f0a":"#091505";
        if(st=="upo")    return alt?"#1f1500":"#170f00";
        if(st=="krit")   return alt?"#1f0800":"#170400";
        if(st=="ist")    return alt?"#200000":"#160000";
        if(st=="neo")    return alt?"#180015":"#10000e";
        return alt?"#1a0e05":"#130900";
    }
    QString cinemaStyle() const { return R"(
* { font-family:'Segoe UI',sans-serif; }
QMainWindow,QDialog { background:#140a02; }
QTabWidget::pane { border:1px solid #4a2800; background:#140a02; border-radius:0 6px 6px 6px; }
QTabBar::tab { background:#0e0600; color:#a07040; padding:11px 28px; border:1px solid #4a2800;
  border-bottom:none; border-top-left-radius:7px; border-top-right-radius:7px;
  font-size:13px; font-weight:600; margin-right:3px; min-width:160px; }
QTabBar::tab:selected { background:#140a02; color:#d4af37; border-bottom:3px solid #d4af37; }
QTabBar::tab:hover:!selected { background:#1a0e04; color:#d4af37; }
QPushButton { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2a1a06,stop:1 #1e1002);
  color:#d4c090; border:1px solid #6a4010; border-radius:7px; padding:9px 20px;
  font-size:13px; font-weight:600; min-width:100px; }
QPushButton:hover { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #3a2a0a,stop:1 #2a1a04);
  border-color:#d4af37; color:#f0d060; }
QPushButton:pressed { background:#1a0e02; }
QPushButton:disabled { color:#4a3010; border-color:#2a1800; background:#120800; }
QPushButton#btnDodaj { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #b8860b,stop:1 #996600); border-color:#d4af37; color:#fff8e0; }
QPushButton#btnObrisi,QPushButton#btnObrisiArh { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #8b0000,stop:1 #6b0000); border-color:#cc0000; color:#ffd0d0; }
QPushButton#btnArhiviraj { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #6b3800,stop:1 #4a2600); border-color:#d4af37; color:#ffe0a0; }
QPushButton#btnVratiArh { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1a4010,stop:1 #0e2c08); border-color:#50a030; color:#c0f0a0; }
QPushButton#btnPDF { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #6b0030,stop:1 #4a0020); border-color:#cc0060; color:#ffd0e0; }
QPushButton#btnCSV { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1a4020,stop:1 #0e2c14); border-color:#40c060; color:#a0f0c0; }
QPushButton#btnUvezi { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #6b4800,stop:1 #4a3200); border-color:#d4af37; color:#ffe8a0; }
QTableWidget { background:#0a0500; gridline-color:transparent; border:1px solid #3a1e00; border-radius:8px; font-size:13px; outline:none; }
QTableWidget::item { border:none; }
QTableWidget::item:selected { background:transparent; }
QHeaderView { background:#0a0500; }
QHeaderView::section { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1e1004,stop:1 #150b02);
  color:#c09030; padding:10px 8px; border:none; border-right:1px solid #3a1e00;
  border-bottom:2px solid #d4af37; font-weight:700; font-size:11px; letter-spacing:0.8px; }
QScrollBar:vertical { background:#0a0500; width:12px; border-radius:6px; }
QScrollBar::handle:vertical { background:#4a2800; border-radius:6px; min-height:30px; }
QScrollBar::handle:vertical:hover { background:#d4af37; }
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
QStatusBar { background:#060300; color:#806030; font-size:12px; border-top:1px solid #3a1e00; padding:3px 8px; }
QLineEdit { background:#0e0800; border:1px solid #4a2800; border-radius:7px; padding:8px 14px; color:#d4c090; font-size:13px; }
QLineEdit:focus { border-color:#d4af37; }
QGroupBox { border:1px solid #4a2800; border-radius:8px; margin-top:14px; font-weight:700; font-size:12px; color:#906030; padding-top:6px; }
QGroupBox::title { subcontrol-origin:margin; left:14px; padding:0 6px; background:#140a02; }
QLabel#lblUpozorenje { color:#d4af37; font-size:13px; font-weight:700; padding:7px 16px; background:#1a0e00; border:1px solid #d4af37; border-radius:7px; }
QLabel#lblKriticno { color:#ff6040; font-size:13px; font-weight:700; padding:7px 16px; background:#1a0000; border:1px solid #cc2200; border-radius:7px; }
QMessageBox { background:#140a02; color:#d4c090; }
QComboBox { background:#0e0800; border:1px solid #4a2800; border-radius:6px; padding:6px 10px; color:#d4c090; }
QComboBox::drop-down { border:none; }
QComboBox QAbstractItemView { background:#0e0800; color:#d4c090; selection-background-color:#8b6914; border:1px solid #4a2800; }
QDateEdit { background:#0e0800; border:1px solid #4a2800; border-radius:4px; padding:5px 10px; color:#d4c090; }
QTextEdit { background:#0e0800; border:1px solid #4a2800; border-radius:4px; padding:6px 10px; color:#d4c090; }
QCheckBox { color:#d4c090; }
QCheckBox::indicator { width:16px; height:16px; border:1px solid #4a2800; border-radius:3px; background:#0a0500; }
QCheckBox::indicator:checked { background:#8b6914; border-color:#d4af37; }
QSpinBox { background:#0e0800; border:1px solid #4a2800; border-radius:6px; padding:6px 10px; color:#d4c090; }
QListWidget { background:#0a0500; border:1px solid #4a2800; border-radius:8px; color:#d4af37; }
QListWidget::item { padding:6px 12px; border-bottom:1px solid #2a1200; }
QListWidget::item:selected { background:#6b4808; color:#fff8e0; border-radius:4px; }
)"; }

    // ── BLUE STEEL ─────────────────────────────────────────────────────────
    QString blueRowBg(const QString &st, bool alt) const {
        if(st=="uredu")  return alt?"#062010":"#041808";
        if(st=="upo")    return alt?"#1a1400":"#120e00";
        if(st=="krit")   return alt?"#1a0c00":"#120600";
        if(st=="ist")    return alt?"#180000":"#100000";
        if(st=="neo")    return alt?"#0e0828":"#080520";
        return alt?"#0a1030":"#060c24";
    }
    QString blueStyle() const { return R"(
* { font-family:'Segoe UI',sans-serif; }
QMainWindow,QDialog { background:#060c20; }
QTabWidget::pane { border:1px solid #0c2060; background:#060c20; border-radius:0 6px 6px 6px; }
QTabBar::tab { background:#04081a; color:#4070b0; padding:11px 28px; border:1px solid #0c2060;
  border-bottom:none; border-top-left-radius:7px; border-top-right-radius:7px;
  font-size:13px; font-weight:600; margin-right:3px; min-width:160px; }
QTabBar::tab:selected { background:#060c20; color:#38bdf8; border-bottom:3px solid #0ea5e9; }
QTabBar::tab:hover:!selected { background:#080e28; color:#60c8f8; }
QPushButton { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #0c2060,stop:1 #081840);
  color:#80b8f0; border:1px solid #1840a0; border-radius:7px; padding:9px 20px;
  font-size:13px; font-weight:600; min-width:100px; }
QPushButton:hover { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1430a0,stop:1 #0c2060);
  border-color:#38bdf8; color:#c0e8ff; }
QPushButton:pressed { background:#081840; }
QPushButton:disabled { color:#1a3060; border-color:#0a1830; background:#04081a; }
QPushButton#btnDodaj { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #0284c7,stop:1 #0369a1); border-color:#38bdf8; color:#e0f4ff; }
QPushButton#btnObrisi,QPushButton#btnObrisiArh { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #7f1d1d,stop:1 #6b1212); border-color:#f87171; color:#fecaca; }
QPushButton#btnArhiviraj { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1040c0,stop:1 #0c3090); border-color:#60a0f0; color:#b0d8ff; }
QPushButton#btnVratiArh { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #0c6040,stop:1 #084030); border-color:#20d080; color:#a0f8d0; }
QPushButton#btnPDF { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #501090,stop:1 #380a70); border-color:#a060f0; color:#e0c8ff; }
QPushButton#btnCSV { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #085040,stop:1 #043828); border-color:#20c080; color:#90f0c8; }
QPushButton#btnUvezi { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #704010,stop:1 #502e08); border-color:#f0a020; color:#ffe8a0; }
QTableWidget { background:#04081a; gridline-color:transparent; border:1px solid #0c2060; border-radius:8px; font-size:13px; outline:none; }
QTableWidget::item { border:none; }
QTableWidget::item:selected { background:transparent; }
QHeaderView { background:#04081a; }
QHeaderView::section { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #0c1840,stop:1 #081028);
  color:#4090e0; padding:10px 8px; border:none; border-right:1px solid #0c2060;
  border-bottom:2px solid #0ea5e9; font-weight:700; font-size:11px; letter-spacing:0.8px; }
QScrollBar:vertical { background:#04081a; width:12px; border-radius:6px; }
QScrollBar::handle:vertical { background:#0c2060; border-radius:6px; min-height:30px; }
QScrollBar::handle:vertical:hover { background:#0ea5e9; }
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
QStatusBar { background:#020408; color:#2050a0; font-size:12px; border-top:1px solid #0c1840; padding:3px 8px; }
QLineEdit { background:#04081a; border:1px solid #0c2060; border-radius:7px; padding:8px 14px; color:#80b8f0; font-size:13px; }
QLineEdit:focus { border-color:#0ea5e9; }
QGroupBox { border:1px solid #0c2060; border-radius:8px; margin-top:14px; font-weight:700; font-size:12px; color:#3070b0; padding-top:6px; }
QGroupBox::title { subcontrol-origin:margin; left:14px; padding:0 6px; background:#060c20; }
QLabel#lblUpozorenje { color:#fbbf24; font-size:13px; font-weight:700; padding:7px 16px; background:#0c0c00; border:1px solid #d97706; border-radius:7px; }
QLabel#lblKriticno { color:#f87171; font-size:13px; font-weight:700; padding:7px 16px; background:#0c0000; border:1px solid #ef4444; border-radius:7px; }
QMessageBox { background:#060c20; color:#80b8f0; }
QComboBox { background:#04081a; border:1px solid #0c2060; border-radius:6px; padding:6px 10px; color:#80b8f0; }
QComboBox::drop-down { border:none; }
QComboBox QAbstractItemView { background:#04081a; color:#80b8f0; selection-background-color:#0369a1; border:1px solid #0c2060; }
QDateEdit { background:#04081a; border:1px solid #0c2060; border-radius:4px; padding:5px 10px; color:#80b8f0; }
QTextEdit { background:#04081a; border:1px solid #0c2060; border-radius:4px; padding:6px 10px; color:#80b8f0; }
QCheckBox { color:#80b8f0; }
QCheckBox::indicator { width:16px; height:16px; border:1px solid #0c2060; border-radius:3px; background:#04081a; }
QCheckBox::indicator:checked { background:#0369a1; border-color:#0ea5e9; }
QSpinBox { background:#04081a; border:1px solid #0c2060; border-radius:6px; padding:6px 10px; color:#80b8f0; }
QListWidget { background:#04081a; border:1px solid #0c2060; border-radius:8px; color:#38bdf8; }
QListWidget::item { padding:6px 12px; border-bottom:1px solid #0a1830; }
QListWidget::item:selected { background:#0369a1; color:#e0f4ff; border-radius:4px; }
)"; }
};

#endif
