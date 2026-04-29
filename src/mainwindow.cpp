#include "mainwindow.h"
#include "filmdialog.h"
#include "lang.h"
#include "theme.h"
#include "settingsdialog.h"

#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QTextCodec>
#include <QStandardPaths>
#include <QDir>
#include <QApplication>
#include <QScreen>
#include <QPalette>
#include <QFrame>
#include <QPrinter>
#include <QFontMetrics>
#include <QDateTime>
#include <QStatusBar>
#include <QTimer>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>

// ─────────────────────────────────────────────────────────────────────────────
//  Boje po statusu
// ─────────────────────────────────────────────────────────────────────────────
static QColor bgForStatus(RowStatus s) {
    switch (s) {
        case RS_ZAKLJUCAN:   return QColor(32, 36, 48);
        case RS_NEOGRANICEN: return QColor(35, 28, 58);
        case RS_UREDU:       return QColor(16, 32, 22);
        case RS_UPOZORENJE:  return QColor(42, 32, 10);
        case RS_KRITICNO:    return QColor(52, 24, 8);
        case RS_ISTEKAO:     return QColor(48, 12, 12);
        case RS_ARHIVA:      return QColor(24, 26, 32);
        default:             return QColor(28, 32, 42);
    }
}
static QColor accentForStatus(RowStatus s) {
    switch (s) {
        case RS_ZAKLJUCAN:   return QColor(80, 95, 120);
        case RS_NEOGRANICEN: return QColor(139, 92, 246);
        case RS_UREDU:       return QColor(34, 197, 94);
        case RS_UPOZORENJE:  return QColor(245, 158, 11);
        case RS_KRITICNO:    return QColor(251, 146, 60);
        case RS_ISTEKAO:     return QColor(239, 68, 68);
        case RS_ARHIVA:      return QColor(60, 70, 90);
        default:             return QColor(96, 165, 250);
    }
}
static QColor fgForStatus(RowStatus s) {
    if (s == RS_ZAKLJUCAN || s == RS_ARHIVA) return QColor(110, 125, 150);
    return QColor(220, 228, 240);
}

// ─────────────────────────────────────────────────────────────────────────────
//  RowDelegate — uniformna selekcija cijelog reda
// ─────────────────────────────────────────────────────────────────────────────
void RowDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt,
                        const QModelIndex &idx) const
{
    p->save();
    QRect r = opt.rect;

    RowStatus st = (RowStatus)idx.data(Qt::UserRole).toInt();
    bool selected = opt.state & QStyle::State_Selected;
    bool hover    = opt.state & QStyle::State_MouseOver;

    QColor bg = selected ? QColor(37, 99, 235)
              : hover    ? bgForStatus(st).lighter(130)
                         : bgForStatus(st);

    // Pozadina
    p->fillRect(r, bg);

    // Lijeva akcentna traka (samo na prvoj koloni)
    if (idx.column() == 0) {
        QColor acc = selected ? QColor(96, 165, 250) : accentForStatus(st);
        p->fillRect(r.x(), r.y(), 4, r.height(), acc);
    }

    // Donja linija separatora
    p->setPen(QColor(38, 44, 58));
    p->drawLine(r.left(), r.bottom(), r.right(), r.bottom());

    // Tekst
    QColor fg = selected ? Qt::white : fgForStatus(st);
    p->setPen(fg);

    QFont f = opt.font;
    // Bold za status kolonu (kolona 9)
    if (idx.column() == 9) { f.setBold(true); f.setPointSize(f.pointSize()); }
    p->setFont(f);

    QString txt = idx.data(Qt::DisplayRole).toString();
    int pad = (idx.column() == 0) ? 14 : 6;
    Qt::Alignment al = (idx.column() == 0 || idx.column() == 10)
                     ? (Qt::AlignVCenter | Qt::AlignLeft)
                     : Qt::AlignCenter;
    p->drawText(r.adjusted(pad, 0, -4, 0), al, txt);

    p->restore();
}

// ─────────────────────────────────────────────────────────────────────────────
//  MainWindow
// ─────────────────────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Valter — Program za evidenciju filmova i kljuceva za bioskop");
    setMinimumSize(1200, 700);
    resize(1400, 800);
    setupStyle();
    setupUI();
    ucitajPodatke();
    popuniTabelu();
    popuniArhivu();
    azurirajStatusBar();
    azurirajBrojacTabova();
    provjeriIstekKljuceva();

    timerProvjera = new QTimer(this);
    connect(timerProvjera, &QTimer::timeout, this, &MainWindow::provjeriIstekKljuceva);
    timerProvjera->start(60000);

    if (auto *screen = QApplication::primaryScreen()) {
        QRect g = screen->availableGeometry();
        move((g.width()-width())/2, (g.height()-height())/2);
    }
}
MainWindow::~MainWindow() { sacuvajPodatke(); }

// ─────────────────────────────────────────────────────────────────────────────
//  Style
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::setupStyle() {
    Theme::instance().apply();
}

// ─────────────────────────────────────────────────────────────────────────────
//  UI Setup
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::setupUI() {
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainL = new QVBoxLayout(central);
    mainL->setSpacing(10);
    mainL->setContentsMargins(16, 14, 16, 10);

    // ── Header ─────────────────────────────────────────────
    QHBoxLayout *hdr = new QHBoxLayout();

    // Logo + title
    QLabel *lblIcon = new QLabel("🎬");
    lblIcon->setStyleSheet("font-size: 32px;");
    QVBoxLayout *ttl = new QVBoxLayout();
    QLabel *lblN = new QLabel("Valter");
    lblN->setStyleSheet("font-size: 24px; font-weight: 900; color: #60a5fa; letter-spacing: 1.5px;");
    QLabel *lblP = new QLabel("Program za evidenciju filmova i kljuceva za bioskop");
    lblP->setStyleSheet("font-size: 12px; color: #3a5080; font-weight: 500;");
    ttl->addWidget(lblN); ttl->addWidget(lblP); ttl->setSpacing(3);
    hdr->addWidget(lblIcon);
    hdr->addSpacing(10);
    hdr->addLayout(ttl);
    hdr->addStretch();

    // Dugme za jezik
    // Settings dugme
    btnSettings = new QPushButton("⚙");
    btnSettings->setToolTip(TR("Podešavanja (tema, jezik)", "Settings (theme, language)"));
    btnSettings->setFixedSize(38, 38);
    btnSettings->setStyleSheet(
        "QPushButton { background:#1e2535; border:1px solid #3d4a62; border-radius:8px;"
        "  font-size:18px; color:#93c5fd; padding:0; }"
        "QPushButton:hover { background:#2d3a55; border-color:#60a5fa; }"
    );
    hdr->addWidget(btnSettings);
    hdr->addSpacing(8);

    lblUpozorenje = new QLabel();
    lblUpozorenje->setObjectName("lblUpozorenje");
    lblUpozorenje->setVisible(false);
    lblUpozorenje->setWordWrap(true);
    lblUpozorenje->setMaximumWidth(700);
    hdr->addWidget(lblUpozorenje);
    mainL->addLayout(hdr);

    // Separator
    QFrame *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
                       "stop:0 #12161e, stop:0.3 #2563eb, stop:0.7 #2563eb, stop:1 #12161e);"
                       "border: none; max-height: 2px; margin: 2px 0;");
    mainL->addWidget(sep);

    // ── Toolbar ────────────────────────────────────────────
    QHBoxLayout *tb = new QHBoxLayout(); tb->setSpacing(8);

    btnDodaj     = new QPushButton("＋  Dodaj Film");   btnDodaj->setObjectName("btnDodaj");
    btnUredi     = new QPushButton("✎  Uredi");
    btnObrisi    = new QPushButton("✕  Obrisi");        btnObrisi->setObjectName("btnObrisi");
    btnArhiviraj = new QPushButton("📁  U Arhivu");     btnArhiviraj->setObjectName("btnArhiviraj");
    btnOsvezi    = new QPushButton("↻  Osvjezi");
    btnPDF       = new QPushButton("📄  PDF");          btnPDF->setObjectName("btnPDF");
    btnCSV       = new QPushButton("📊  Excel/CSV");    btnCSV->setObjectName("btnCSV");
    btnUvezi     = new QPushButton("📥  Uvezi");        btnUvezi->setObjectName("btnUvezi");

    btnUredi->setEnabled(false);
    btnObrisi->setEnabled(false);
    btnArhiviraj->setEnabled(false);

    tb->addWidget(btnDodaj); tb->addWidget(btnUredi); tb->addWidget(btnObrisi);
    tb->addWidget(btnArhiviraj); tb->addSpacing(8); tb->addWidget(btnOsvezi);
    tb->addSpacing(12); tb->addWidget(btnPDF); tb->addWidget(btnCSV); tb->addWidget(btnUvezi);
    tb->addStretch();

    // Pretraga
    QLabel *lblSrch = new QLabel("🔍");
    lblSrch->setStyleSheet("font-size: 15px;");
    edPretraga = new QLineEdit();
    edPretraga->setPlaceholderText("Pretrazi filmove...");
    edPretraga->setMinimumWidth(240);
    edPretraga->setMaximumWidth(300);
    tb->addWidget(lblSrch); tb->addWidget(edPretraga);
    mainL->addLayout(tb);

    // ── Legenda ────────────────────────────────────────────
    QHBoxLayout *leg = new QHBoxLayout(); leg->setSpacing(6);
    leg->addStretch();
    auto mkLeg = [&](const QString &col, const QString &lbl) {
        QLabel *dot = new QLabel();
        dot->setFixedSize(12, 12);
        dot->setStyleSheet(QString("background:%1; border-radius:6px;").arg(col));
        QLabel *txt = new QLabel(lbl);
        txt->setStyleSheet("color: #4a5e85; font-size: 12px;");
        leg->addWidget(dot); leg->addWidget(txt); leg->addSpacing(8);
    };
    mkLeg("#22c55e", "U redu");
    mkLeg("#f59e0b", "Upozorenje (≤7d)");
    mkLeg("#f97316", "Kriticno (≤1d)");
    mkLeg("#ef4444", "Istekao");
    mkLeg("#a855f7", "Neogranicen");
    mkLeg("#5a6f90", "Zakljucan");
    mainL->addLayout(leg);

    // ── Tab widget ─────────────────────────────────────────
    tabWidget = new QTabWidget(this);
    tabWidget->setDocumentMode(false);
    mainL->addWidget(tabWidget, 1);

    // Tab 1 — Aktivni
    QWidget *pgAkt = new QWidget();
    QVBoxLayout *laAkt = new QVBoxLayout(pgAkt);
    laAkt->setContentsMargins(0, 8, 0, 0); laAkt->setSpacing(0);
    tabela = new QTableWidget();
    tabela->setColumnCount(11);
    tabela->setHorizontalHeaderLabels({
        "Naziv Filma","Format","Kljuc","Od","Do",
        "Trajanje","Preostalo","Skinut","Injestovan","Status","Napomena"
    });
    setupTabela(tabela);
    laAkt->addWidget(tabela);
    tabWidget->addTab(pgAkt, "  Aktivni filmovi");

    // Tab 2 — Arhiva
    QWidget *pgArh = new QWidget();
    QVBoxLayout *laArh = new QVBoxLayout(pgArh);
    laArh->setContentsMargins(0, 8, 0, 0); laArh->setSpacing(8);

    QHBoxLayout *arhTb = new QHBoxLayout(); arhTb->setSpacing(8);
    btnVratiIzArhive = new QPushButton("↩  Vrati u aktivne"); btnVratiIzArhive->setObjectName("btnVratiArh");
    btnObrisiArhiva  = new QPushButton("✕  Trajno obrisi");   btnObrisiArhiva->setObjectName("btnObrisiArh");
    btnVratiIzArhive->setEnabled(false); btnObrisiArhiva->setEnabled(false);
    QLabel *lblArhInfo = new QLabel("Filmovi kojima je istekao kljuc ili su rucno premjesteni.");
    lblArhInfo->setStyleSheet("color: #3a4e6a; font-size: 12px; font-style: italic;");
    arhTb->addWidget(btnVratiIzArhive); arhTb->addWidget(btnObrisiArhiva);
    arhTb->addSpacing(16); arhTb->addWidget(lblArhInfo); arhTb->addStretch();
    laArh->addLayout(arhTb);

    tabelaArhiva = new QTableWidget();
    tabelaArhiva->setColumnCount(11);
    tabelaArhiva->setHorizontalHeaderLabels({
        "Naziv Filma","Format","Kljuc","Od","Do",
        "Trajanje","Preostalo","Skinut","Injestovan","Status","Napomena"
    });
    setupTabela(tabelaArhiva);
    laArh->addWidget(tabelaArhiva);
    tabWidget->addTab(pgArh, "  Arhiva");

    statusBar()->showMessage("Spreman  |  KC Bar");

    // Konekcije
    connect(btnDodaj,         &QPushButton::clicked, this, &MainWindow::dodajFilm);
    connect(btnUredi,         &QPushButton::clicked, this, &MainWindow::urediFilm);
    connect(btnObrisi,        &QPushButton::clicked, this, &MainWindow::obrisiFilm);
    connect(btnArhiviraj,     &QPushButton::clicked, this, &MainWindow::posaljiUArhivu);
    connect(btnVratiIzArhive, &QPushButton::clicked, this, &MainWindow::vratiIzArhive);
    connect(btnObrisiArhiva,  &QPushButton::clicked, this, &MainWindow::obrisiIzArhive);
    connect(btnOsvezi,        &QPushButton::clicked, this, &MainWindow::osveziTabelu);
    connect(btnPDF,           &QPushButton::clicked, this, &MainWindow::izvezuPDF);
    connect(btnCSV,           &QPushButton::clicked, this, &MainWindow::izvezuCSV);
    connect(btnUvezi,         &QPushButton::clicked, this, &MainWindow::uvezi);
    connect(edPretraga,       &QLineEdit::textChanged, this, &MainWindow::pretragaPromijenjena);
    connect(tabela,       &QTableWidget::itemSelectionChanged, this, &MainWindow::selekcijaPromijenjena);
    connect(tabela,       &QTableWidget::cellDoubleClicked,    this, &MainWindow::urediFilm);
    connect(tabelaArhiva, &QTableWidget::itemSelectionChanged, this, &MainWindow::selekcijaArhivePromijenjena);
    connect(tabWidget,    &QTabWidget::currentChanged,         this, &MainWindow::tabPromijenjena);
    connect(btnSettings,  &QPushButton::clicked,               this, &MainWindow::openSettings);
}

void MainWindow::setupTabela(QTableWidget *t) {
    // Delegat za uniformno bojanje
    t->setItemDelegate(new RowDelegate(t));
    t->setMouseTracking(true);

    // Sve kolone su Interactive — korisnik moze da vuce i mijenja sirinu
    for (int i = 0; i < 11; i++)
        t->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Interactive);

    // Naziv i Napomena se istežu, ostale fiksne default sirine
    t->horizontalHeader()->setSectionResizeMode(0,  QHeaderView::Stretch);
    t->horizontalHeader()->setSectionResizeMode(10, QHeaderView::Stretch);

    // Default sirine — dovoljno da se vidi pun tekst
    t->setColumnWidth(1,  92);   // Format
    t->setColumnWidth(2,  118);  // Kljuc — "Otkljucan" = 9 znakova
    t->setColumnWidth(3,  108);  // Od
    t->setColumnWidth(4,  108);  // Do
    t->setColumnWidth(5,  100);  // Trajanje
    t->setColumnWidth(6,  118);  // Preostalo
    t->setColumnWidth(7,   82);  // Skinut
    t->setColumnWidth(8,   96);  // Injestovan
    t->setColumnWidth(9,  118);  // Status

    // Minimalne sirine da se ne može smanjiti ispod čitljivog
    t->horizontalHeader()->setMinimumSectionSize(60);

    t->verticalHeader()->setVisible(false);
    t->setAlternatingRowColors(false); // Delegat preuzima bojanje
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSortingEnabled(true);
    t->setShowGrid(false);
    t->verticalHeader()->setDefaultSectionSize(44);
    t->horizontalHeader()->setCursor(Qt::SizeHorCursor);
    t->setStyleSheet("QTableWidget { border-radius: 8px; }"
                     "QHeaderView::section { cursor: col-resize; }");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────
RowStatus MainWindow::rowStatus(const Film &f) const {
    if (f.uArhivi)          return RS_ARHIVA;
    if (!f.kljucOtkljucan)  return RS_ZAKLJUCAN;
    if (f.bezOgranicenja)   return RS_NEOGRANICEN;
    int d = danaDoIsteka(f);
    if (d < 0)  return RS_ISTEKAO;
    if (d <= 1) return RS_KRITICNO;
    if (d <= 7) return RS_UPOZORENJE;
    return RS_UREDU;
}

int MainWindow::danaDoIsteka(const Film &f) const {
    if (!f.kljucOtkljucan || f.bezOgranicenja) return 99999;
    return QDate::currentDate().daysTo(f.isticaKljuca);
}
int MainWindow::trajanjeDana(const Film &f) const {
    if (f.bezOgranicenja || !f.datumOd.isValid() || !f.isticaKljuca.isValid()) return 0;
    return f.datumOd.daysTo(f.isticaKljuca);
}
QString MainWindow::statusKljuca(const Film &f) const {
    if (!f.kljucOtkljucan)  return "Zakljucan";
    if (f.bezOgranicenja)   return "Neogranicen";
    int d = danaDoIsteka(f);
    if (d < 0)  return QString("Istekao (%1d)").arg(-d);
    if (d == 0) return "DANAS istice!";
    if (d == 1) return "SUTRA istice!";
    if (d <= 7) return QString("%1 dan(a)").arg(d);
    return QString("U redu (%1d)").arg(d);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Popuni red — samo tekst + UserRole, delegat boji sve
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::popuniRed(QTableWidget *tbl, int row, const Film &f) {
    RowStatus st = rowStatus(f);
    int dana = danaDoIsteka(f);
    int traj = trajanjeDana(f);

    auto mk = [&](const QString &txt) {
        QTableWidgetItem *it = new QTableWidgetItem(txt);
        it->setData(Qt::UserRole, (int)st);
        return it;
    };

    // Format boja (extra info u UserRole+1 nije podrzana, koristimo poseban role)
    auto mkFmt = [&](const QString &txt) {
        QTableWidgetItem *it = new QTableWidgetItem(txt);
        it->setData(Qt::UserRole, (int)st);
        // Sacuvaj format info za delegata
        it->setData(Qt::UserRole + 1, txt.contains("4K") ? 1 : 0);
        return it;
    };

    // 0 - Naziv
    tbl->setItem(row, 0, mk(f.naziv));

    // 1 - Format
    tbl->setItem(row, 1, mkFmt(f.format));

    // 2 - Kljuc — puni naziv uvijek
    QString kTxt = !f.kljucOtkljucan ? "Zakljucan"
                 : f.bezOgranicenja  ? "Neogranicen"
                                     : "Otkljucan";
    tbl->setItem(row, 2, mk(kTxt));

    // 3 - Od
    tbl->setItem(row, 3, mk(
        (!f.kljucOtkljucan || f.bezOgranicenja) ? "—"
        : f.datumOd.toString("dd.MM.yyyy")));

    // 4 - Do
    tbl->setItem(row, 4, mk(
        (!f.kljucOtkljucan || f.bezOgranicenja) ? "—"
        : f.isticaKljuca.toString("dd.MM.yyyy")));

    // 5 - Trajanje
    tbl->setItem(row, 5, mk(
        !f.kljucOtkljucan ? "—"
        : f.bezOgranicenja ? "∞"
        : QString("%1 dana").arg(traj)));

    // 6 - Preostalo
    QString danaTxt;
    if (!f.kljucOtkljucan)    danaTxt = "—";
    else if (f.bezOgranicenja) danaTxt = "∞";
    else if (dana < 0)         danaTxt = QString("Istekao  %1d").arg(-dana);
    else if (dana == 0)        danaTxt = "⚠  DANAS!";
    else if (dana == 1)        danaTxt = "⚠  SUTRA!";
    else if (dana <= 7)        danaTxt = QString("⚠  %1 dan(a)").arg(dana);
    else                       danaTxt = QString("%1 dana").arg(dana);
    tbl->setItem(row, 6, mk(danaTxt));

    // 7 - Skinut
    tbl->setItem(row, 7, mk(f.filmSkinut ? "✔  Da" : "✘  Ne"));

    // 8 - Injestovan
    tbl->setItem(row, 8, mk(f.filmInjestovan ? "✔  Da" : "✘  Ne"));

    // 9 - Status
    QString stTxt;
    switch (st) {
        case RS_ZAKLJUCAN:   stTxt = "ZAKLJUCAN";   break;
        case RS_NEOGRANICEN: stTxt = "NEOGRANICEN";  break;
        case RS_UREDU:       stTxt = "U REDU"; break;
        case RS_UPOZORENJE:  stTxt = "UPOZORENJE"; break;
        case RS_KRITICNO:    stTxt = "KRITICNO!"; break;
        case RS_ISTEKAO:     stTxt = "ISTEKAO"; break;
        case RS_ARHIVA:      stTxt = "ARHIVA";        break;
    }
    tbl->setItem(row, 9, mk(stTxt));

    // 10 - Napomena
    tbl->setItem(row, 10, mk(f.napomena));
}

void MainWindow::popuniTabelu(const QString &filter) {
    tabela->setSortingEnabled(false);
    QList<Film*> lista;
    for (auto &f : filmovi)
        if (!f.uArhivi && (filter.isEmpty() || f.naziv.contains(filter, Qt::CaseInsensitive)))
            lista.append(&f);
    tabela->setRowCount(lista.size());
    for (int i = 0; i < lista.size(); ++i)
        popuniRed(tabela, i, *lista[i]);
    tabela->setSortingEnabled(true);
}

void MainWindow::popuniArhivu(const QString &filter) {
    tabelaArhiva->setSortingEnabled(false);
    QList<Film*> lista;
    for (auto &f : filmovi)
        if (f.uArhivi && (filter.isEmpty() || f.naziv.contains(filter, Qt::CaseInsensitive)))
            lista.append(&f);
    tabelaArhiva->setRowCount(lista.size());
    for (int i = 0; i < lista.size(); ++i)
        popuniRed(tabelaArhiva, i, *lista[i]);
    tabelaArhiva->setSortingEnabled(true);
}

void MainWindow::azurirajStatusBar() {
    int uk=0, otk=0, upo=0, ist=0, arh=0;
    for (const auto &f : filmovi) {
        if (f.uArhivi) { arh++; continue; }
        uk++;
        if (!f.kljucOtkljucan || f.bezOgranicenja) continue;
        otk++;
        int d = danaDoIsteka(f);
        if (d < 0) ist++; else if (d <= 7) upo++;
    }
    statusBar()->showMessage(
        QString("  Aktivnih: %1   |   Otkljucanih: %2   |   Upozorenja: %3   |   Isteklih: %4   |   Arhiva: %5   |   Valter - KC Bar")
        .arg(uk).arg(otk).arg(upo).arg(ist).arg(arh));
}

void MainWindow::azurirajBrojacTabova() {
    int akt=0, arh=0;
    for (const auto &f : filmovi) { if (f.uArhivi) arh++; else akt++; }
    tabWidget->setTabText(0, QString("  Aktivni filmovi  (%1)  ").arg(akt));
    tabWidget->setTabText(1, QString("  Arhiva  (%1)  ").arg(arh));
}

void MainWindow::provjeriIstekKljuceva() {
    QStringList kriticno, upoz;
    for (const auto &f : filmovi) {
        if (f.uArhivi || !f.kljucOtkljucan || f.bezOgranicenja) continue;
        int d = danaDoIsteka(f);
        if (d < 0)       kriticno << QString("%1 (istekao %2d)").arg(f.naziv).arg(-d);
        else if (d == 0) kriticno << QString("%1 (DANAS!)").arg(f.naziv);
        else if (d == 1) kriticno << QString("%1 (SUTRA!)").arg(f.naziv);
        else if (d <= 7) upoz     << QString("%1 (%2d)").arg(f.naziv).arg(d);
    }
    if (!kriticno.isEmpty()) {
        lblUpozorenje->setObjectName("lblKriticno");
        lblUpozorenje->setStyle(lblUpozorenje->style());
        lblUpozorenje->setText("KRITICNO: " + kriticno.join("   |   "));
        lblUpozorenje->setVisible(true);
    } else if (!upoz.isEmpty()) {
        lblUpozorenje->setObjectName("lblUpozorenje");
        lblUpozorenje->setStyle(lblUpozorenje->style());
        lblUpozorenje->setText("UPOZORENJE: " + upoz.join("   |   "));
        lblUpozorenje->setVisible(true);
    } else {
        lblUpozorenje->setVisible(false);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  CRUD
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::dodajFilm() {
    FilmDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        filmovi.append(dlg.getFilm());
        sacuvajPodatke(); popuniTabelu(); popuniArhivu();
        azurirajStatusBar(); azurirajBrojacTabova(); provjeriIstekKljuceva();
    }
}
void MainWindow::urediFilm() {
    int row = tabela->currentRow();
    if (row < 0) return;
    QString naziv = tabela->item(row, 0)->text().trimmed();
    for (auto &f : filmovi) {
        if (!f.uArhivi && f.naziv == naziv) {
            FilmDialog dlg(this, &f);
            if (dlg.exec() == QDialog::Accepted) {
                bool bio_arhiva = f.uArhivi;
                f = dlg.getFilm(); f.uArhivi = bio_arhiva;
                sacuvajPodatke(); popuniTabelu(edPretraga->text());
                azurirajStatusBar(); azurirajBrojacTabova(); provjeriIstekKljuceva();
            }
            return;
        }
    }
}
void MainWindow::obrisiFilm() {
    int row = tabela->currentRow();
    if (row < 0) return;
    QString naziv = tabela->item(row, 0)->text().trimmed();
    if (QMessageBox::question(this, "Brisanje filma",
            QString("Obrisati film <b>%1</b>?").arg(naziv),
            QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {
        filmovi.erase(std::remove_if(filmovi.begin(), filmovi.end(),
            [&](const Film &f){ return !f.uArhivi && f.naziv == naziv; }), filmovi.end());
        sacuvajPodatke(); popuniTabelu(edPretraga->text());
        azurirajStatusBar(); azurirajBrojacTabova(); provjeriIstekKljuceva();
    }
}
void MainWindow::posaljiUArhivu() {
    int row = tabela->currentRow();
    if (row < 0) return;
    QString naziv = tabela->item(row, 0)->text().trimmed();
    for (auto &f : filmovi) {
        if (!f.uArhivi && f.naziv == naziv) {
            f.uArhivi = true;
            sacuvajPodatke(); popuniTabelu(edPretraga->text()); popuniArhivu();
            azurirajStatusBar(); azurirajBrojacTabova(); provjeriIstekKljuceva();
            statusBar()->showMessage(QString("Film \"%1\" premjesten u arhivu.").arg(naziv), 4000);
            return;
        }
    }
}
void MainWindow::vratiIzArhive() {
    int row = tabelaArhiva->currentRow();
    if (row < 0) return;
    QString naziv = tabelaArhiva->item(row, 0)->text().trimmed();
    for (auto &f : filmovi) {
        if (f.uArhivi && f.naziv == naziv) {
            f.uArhivi = false;
            sacuvajPodatke(); popuniTabelu(); popuniArhivu();
            azurirajStatusBar(); azurirajBrojacTabova(); provjeriIstekKljuceva();
            statusBar()->showMessage(QString("Film \"%1\" vracen u aktivne.").arg(naziv), 4000);
            return;
        }
    }
}
void MainWindow::obrisiIzArhive() {
    int row = tabelaArhiva->currentRow();
    if (row < 0) return;
    QString naziv = tabelaArhiva->item(row, 0)->text().trimmed();
    if (QMessageBox::question(this, "Trajno brisanje",
            QString("Trajno obrisati <b>%1</b> iz arhive?").arg(naziv),
            QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {
        filmovi.erase(std::remove_if(filmovi.begin(), filmovi.end(),
            [&](const Film &f){ return f.uArhivi && f.naziv == naziv; }), filmovi.end());
        sacuvajPodatke(); popuniArhivu();
        azurirajStatusBar(); azurirajBrojacTabova();
    }
}
void MainWindow::osveziTabelu() {
    popuniTabelu(edPretraga->text()); popuniArhivu(edPretraga->text());
    azurirajStatusBar(); azurirajBrojacTabova(); provjeriIstekKljuceva();
}
void MainWindow::selekcijaPromijenjena() {
    bool s = tabela->currentRow() >= 0;
    btnUredi->setEnabled(s); btnObrisi->setEnabled(s); btnArhiviraj->setEnabled(s);
}
void MainWindow::selekcijaArhivePromijenjena() {
    bool s = tabelaArhiva->currentRow() >= 0;
    btnVratiIzArhive->setEnabled(s); btnObrisiArhiva->setEnabled(s);
}
void MainWindow::pretragaPromijenjena(const QString &txt) {
    popuniTabelu(txt); popuniArhivu(txt);
}
void MainWindow::tabPromijenjena(int) {
    btnUredi->setEnabled(false); btnObrisi->setEnabled(false); btnArhiviraj->setEnabled(false);
    btnVratiIzArhive->setEnabled(false); btnObrisiArhiva->setEnabled(false);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Export PDF — mm koordinatni sistem, bez scale()
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::izvezuPDF() {
    QString path = QFileDialog::getSaveFileName(this,
        TR("Izvezi PDF","Export PDF"),
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
        + "/Valter_" + QDate::currentDate().toString("dd-MM-yyyy") + ".pdf",
        "PDF (*.pdf)");
    if (path.isEmpty()) return;

    QPrinter pr(QPrinter::HighResolution);
    pr.setOutputFormat(QPrinter::PdfFormat);
    pr.setOutputFileName(path);
    pr.setPageSize(QPageSize(QPageSize::A4));
    pr.setPageOrientation(QPageLayout::Landscape);
    pr.setPageMargins(QMarginsF(4,4,4,4), QPageLayout::Millimeter);
    pr.setFullPage(false);

    QPainter p;
    if (!p.begin(&pr)) {
        QMessageBox::warning(this, TR("Greska","Error"),
            TR("Nije moguce kreirati PDF.","Cannot create PDF.")); return;
    }

    // Sve koordinate u mm, konvertujemo u device pixels
    // 1 mm = resolution / 25.4 pixels
    const double res = pr.resolution();
    const double mm  = res / 25.4;

    // Dimenzije stranice u mm (A4 landscape sa 6mm marginama)
    // A4 landscape = 297 x 210 mm, minus 12mm margine = 285 x 198
    const double W = 289.0;
    const double H = 202.0;

    auto px  = [&](double v_mm) -> qreal { return v_mm * mm; };
    auto pxR = [&](double x, double y, double w, double h) -> QRectF {
        return QRectF(px(x), px(y), px(w), px(h));
    };
    auto fillR = [&](QPainter &pt, double x, double y, double w, double h, const QColor &col) {
        pt.fillRect(pxR(x,y,w,h), col);
    };

    // ── Bijela pozadina ─────────────────────────────────────────
    p.fillRect(QRectF(0, 0, px(W), px(H)), Qt::white);

    // ── Header (visina 18mm) ─────────────────────────────────────
    const double hH = 18.0;
    fillR(p, 0, 0, W, hH, QColor(15,23,42));
    fillR(p, 0, 0, 1.5, hH, QColor(212,175,55));   // zlatna traka
    fillR(p, 0, hH, W, 0.7, QColor(37,99,235));     // plava linija

    // Naslov
    QFont fT("Arial", 13, QFont::Bold);
    p.setFont(fT); p.setPen(QColor(96,165,250));
    p.drawText(pxR(3, 1.5, W-70, 8), Qt::AlignVCenter|Qt::AlignLeft, "VALTER");

    QFont fS("Arial", 6.5);
    p.setFont(fS); p.setPen(QColor(80,110,170));
    p.drawText(pxR(3, 9.5, W-70, 7),
        Qt::AlignVCenter|Qt::AlignLeft,
        TR("Program za evidenciju filmova i kljuceva za bioskop  —  KC Bar",
           "Film and Key Management Program  —  KC Bar"));

    // Statistika desno
    int uk=0,upo=0,ist=0;
    for (const auto &f : filmovi) {
        if (f.uArhivi) continue; uk++;
        if (!f.kljucOtkljucan||f.bezOgranicenja) continue;
        int d=danaDoIsteka(f); if(d<0)ist++; else if(d<=7)upo++;
    }
    QFont fSt("Arial", 6);
    p.setFont(fSt); p.setPen(QColor(60,90,140));
    p.drawText(pxR(W-68, 2, 65, 7), Qt::AlignVCenter|Qt::AlignRight,
        TR(QString("Aktivnih: %1  Upoz.: %2  Isteklih: %3").arg(uk).arg(upo).arg(ist),
           QString("Active: %1  Warn.: %2  Expired: %3").arg(uk).arg(upo).arg(ist)));
    p.drawText(pxR(W-68, 9.5, 65, 7), Qt::AlignVCenter|Qt::AlignRight,
        QDateTime::currentDateTime().toString("dd.MM.yyyy  hh:mm"));

    double y = hH + 0.7 + 3.0;

    // ── Kolone (ukupno = W mm) ───────────────────────────────────
    struct Col { QString hdr; double w; Qt::Alignment al; };
    QVector<Col> cols = {
        {TR("NAZIV FILMA","FILM TITLE"), 46.0, Qt::AlignLeft  |Qt::AlignVCenter},
        {TR("FORMAT","FORMAT"),          16.0, Qt::AlignCenter|Qt::AlignVCenter},
        {TR("KLJUC","KEY"),              19.0, Qt::AlignCenter|Qt::AlignVCenter},
        {TR("OD","FROM"),                18.0, Qt::AlignCenter|Qt::AlignVCenter},
        {TR("DO","TO"),                  18.0, Qt::AlignCenter|Qt::AlignVCenter},
        {TR("TRAJANJE","DURAT."),        16.0, Qt::AlignCenter|Qt::AlignVCenter},
        {TR("PREOSTALO","REMAIN."),      19.0, Qt::AlignCenter|Qt::AlignVCenter},
        {TR("SKINUT","DWNLD"),           13.0, Qt::AlignCenter|Qt::AlignVCenter},
        {TR("INJEST.","INGEST."),        13.0, Qt::AlignCenter|Qt::AlignVCenter},
        {TR("STATUS","STATUS"),          19.0, Qt::AlignCenter|Qt::AlignVCenter},
        {TR("NAPOMENA","NOTE"),           0.0, Qt::AlignLeft  |Qt::AlignVCenter},
    };
    // Napomena dobija ostatak prostora
    double usedW = 0; for(int i=0;i<10;i++) usedW += cols[i].w;
    cols[10].w = W - usedW - 0.5;  // ostatak do kraja stranice

    const double rowH = 6.5;  // visina reda u mm
    const double pad  = 1.2;  // padding u mm

    // ── Zaglavlje tabele ─────────────────────────────────────────
    fillR(p, 0, y, W, rowH, QColor(25,38,70));
    fillR(p, 0, y+rowH-0.5, W, 0.5, QColor(37,99,235));

    QFont fH("Arial", 5.5, QFont::Bold);
    p.setFont(fH); p.setPen(QColor(130,165,220));
    double cx = 1.5;
    for (int i=0; i<cols.size(); i++) {
        p.save();
        p.setClipRect(pxR(cx, y, cols[i].w-0.5, rowH));
        p.drawText(pxR(cx, y, cols[i].w-0.5, rowH), cols[i].al, cols[i].hdr);
        p.restore();
        if (i < cols.size()-1)
            fillR(p, cx+cols[i].w-0.3, y+1.5, 0.2, rowH-3.0, QColor(45,65,110));
        cx += cols[i].w;
    }
    y += rowH;

    // ── Redovi ───────────────────────────────────────────────────
    QFont fD("Arial", 6);
    QFont fDB("Arial", 6, QFont::Bold);
    bool alt = false;

    for (const auto &f : filmovi) {
        if (f.uArhivi) continue;

        // Nova stranica
        if (y + rowH > H - 6.0) {
            pr.newPage();
            p.fillRect(QRectF(0,0,px(W),px(H)), Qt::white);
            y = 4.0;
        }

        RowStatus st = rowStatus(f);
        QColor bgRow, acc;
        switch(st) {
            case RS_UREDU:       bgRow=QColor(240,253,244); acc=QColor(34,197,94);   break;
            case RS_UPOZORENJE:  bgRow=QColor(254,252,232); acc=QColor(234,179,8);   break;
            case RS_KRITICNO:    bgRow=QColor(255,247,237); acc=QColor(249,115,22);  break;
            case RS_ISTEKAO:     bgRow=QColor(254,242,242); acc=QColor(239,68,68);   break;
            case RS_NEOGRANICEN: bgRow=QColor(250,245,255); acc=QColor(168,85,247);  break;
            default:             bgRow=QColor(248,250,252); acc=QColor(148,163,184); break;
        }
        if (alt) bgRow = bgRow.darker(104);
        alt = !alt;

        fillR(p, 0, y, W, rowH, bgRow);
        fillR(p, 0, y, 1.2, rowH, acc);
        fillR(p, 0, y+rowH-0.2, W, 0.2, QColor(210,220,235));

        int dana = danaDoIsteka(f);
        QString pre;
        if (!f.kljucOtkljucan)    pre="—";
        else if (f.bezOgranicenja) pre="∞";
        else if (dana<0)           pre=TR(QString("Isteklo %1d").arg(-dana),QString("Exp. %1d").arg(-dana));
        else if (dana==0)          pre=TR("DANAS!","TODAY!");
        else if (dana==1)          pre=TR("SUTRA!","TOMOR.!");
        else                       pre=TR(QString("%1d").arg(dana),QString("%1d").arg(dana));

        QString kl  = !f.kljucOtkljucan?TR("Zakljucan","Locked"):f.bezOgranicenja?TR("Neogr.","Unlim."):TR("Otkljucan","Unlocked");
        QString od  = (!f.kljucOtkljucan||f.bezOgranicenja)?"—":f.datumOd.toString("dd.MM.yy");
        QString doo = (!f.kljucOtkljucan||f.bezOgranicenja)?"—":f.isticaKljuca.toString("dd.MM.yy");
        QString trj = !f.kljucOtkljucan?"—":f.bezOgranicenja?"∞":QString("%1d").arg(trajanjeDana(f));
        QString stT;
        switch(st){
            case RS_UREDU:       stT=TR("U REDU","OK");       break;
            case RS_UPOZORENJE:  stT=TR("UPOZ.","WARN.");     break;
            case RS_KRITICNO:    stT=TR("KRIT.","CRIT.");     break;
            case RS_ISTEKAO:     stT=TR("ISTEKAO","EXPIRED"); break;
            case RS_NEOGRANICEN: stT=TR("NEOGR.","UNLIM.");   break;
            default:             stT=TR("ZAKLJ.","LOCKED");   break;
        }

        QStringList vals = {
            f.naziv, f.format, kl, od, doo, trj, pre,
            f.filmSkinut?TR("Da","Yes"):TR("Ne","No"),
            f.filmInjestovan?TR("Da","Yes"):TR("Ne","No"),
            stT, f.napomena
        };

        cx = 1.5 + pad;
        for (int i=0; i<cols.size(); i++) {
            QColor fg;
            if (i==9) {
                fg=acc; p.setFont(fDB);
            } else if (i==7||i==8) {
                bool da=(vals[i]==TR("Da","Yes"));
                fg=da?QColor(21,128,61):QColor(185,28,28);
                p.setFont(fDB);
            } else if (i==0) {
                fg=QColor(15,23,42); p.setFont(fDB);
            } else {
                fg=QColor(51,65,85); p.setFont(fD);
            }
            p.setPen(fg);
            p.save();
            p.setClipRect(pxR(cx-pad, y, cols[i].w-0.4, rowH));
            p.drawText(pxR(cx-pad, y, cols[i].w-0.6, rowH), cols[i].al, vals[i]);
            p.restore();
            cx += cols[i].w;
        }
        y += rowH;
    }

    // ── Footer ───────────────────────────────────────────────────
    fillR(p, 0, H-5.0, W, 5.0, QColor(241,245,249));
    fillR(p, 0, H-5.0, W, 0.3, QColor(200,215,235));
    QFont fF("Arial", 5.5);
    p.setFont(fF); p.setPen(QColor(120,145,185));
    p.drawText(pxR(2, H-4.5, W/2, 4),
        Qt::AlignVCenter|Qt::AlignLeft,
        "Valter — " + TR("Evidencija filmova i kljuceva","Film and Key Management") + " — KC Bar");
    p.drawText(pxR(W/2, H-4.5, W/2-2, 4),
        Qt::AlignVCenter|Qt::AlignRight,
        QDateTime::currentDateTime().toString("dd.MM.yyyy  hh:mm"));

    p.end();
    QMessageBox::information(this,
        TR("PDF Izvezen","PDF Exported"),
        TR(QString("PDF sacuvan:\n%1").arg(path),
           QString("PDF saved:\n%1").arg(path)));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Export Excel (SpreadsheetML — bez upozorenja) i CSV
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::izvezuCSV() {
    QMessageBox fmtBox(this);
    fmtBox.setWindowTitle(TR("Odaberi format", "Choose format"));
    fmtBox.setText(TR("Odaberite format za export:", "Choose export format:"));
    QPushButton *btnXls = fmtBox.addButton(
        TR("  Excel (.xlsx)","  Excel (.xlsx)"), QMessageBox::AcceptRole);
    QPushButton *btnCsv = fmtBox.addButton(
        TR("  CSV (.csv)","  CSV (.csv)"), QMessageBox::AcceptRole);
    fmtBox.addButton(TR("Otkazi","Cancel"), QMessageBox::RejectRole);
    fmtBox.exec();
    bool doExcel = (fmtBox.clickedButton() == btnXls);
    bool doCsv   = (fmtBox.clickedButton() == btnCsv);
    if (!doExcel && !doCsv) return;

    if (doExcel) {
        // ── SpreadsheetML XML — Excel otvara BEZ upozorenja ──────────────
        // Ovo je pravi Excel 2003 XML format koji Excel prepoznaje nativno
        QString path = QFileDialog::getSaveFileName(this,
            TR("Izvezi Excel","Export Excel"),
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
            + "/Valter_" + QDate::currentDate().toString("dd-MM-yyyy") + ".xml",
            TR("Excel XML (*.xml)","Excel XML (*.xml)"));
        if (path.isEmpty()) return;

        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream o(&f);
        o.setCodec("UTF-8");

        int uk=0, upo=0, ist=0;
        for (const auto &fi : filmovi) {
            if (fi.uArhivi) continue; uk++;
            if (!fi.kljucOtkljucan || fi.bezOgranicenja) continue;
            int d=danaDoIsteka(fi); if(d<0)ist++; else if(d<=7)upo++;
        }

        // SpreadsheetML header
        o << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        o << "<?mso-application progid=\"Excel.Sheet\"?>\n";
        o << "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"\n";
        o << " xmlns:o=\"urn:schemas-microsoft-com:office:office\"\n";
        o << " xmlns:x=\"urn:schemas-microsoft-com:office:excel\"\n";
        o << " xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\"\n";
        o << " xmlns:html=\"http://www.w3.org/TR/REC-html40\">\n";

        // Stilovi
        o << "<Styles>\n";
        // 0 - Default
        o << "<Style ss:ID=\"s0\"><Alignment ss:Horizontal=\"Left\" ss:Vertical=\"Center\"/>"
             "<Font ss:FontName=\"Calibri\" ss:Size=\"10\"/></Style>\n";
        // 1 - Header naslov
        o << "<Style ss:ID=\"s1\">"
             "<Alignment ss:Horizontal=\"Left\" ss:Vertical=\"Center\"/>"
             "<Font ss:FontName=\"Calibri\" ss:Size=\"14\" ss:Bold=\"1\" ss:Color=\"#1D4ED8\"/>"
             "<Interior ss:Color=\"#0F172A\" ss:Pattern=\"Solid\"/>"
             "<Borders><Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"3\" ss:Color=\"#2563EB\"/></Borders>"
             "</Style>\n";
        // 2 - Sub header
        o << "<Style ss:ID=\"s2\">"
             "<Alignment ss:Horizontal=\"Left\" ss:Vertical=\"Center\"/>"
             "<Font ss:FontName=\"Calibri\" ss:Size=\"9\" ss:Color=\"#475569\"/>"
             "<Interior ss:Color=\"#0F172A\" ss:Pattern=\"Solid\"/>"
             "</Style>\n";
        // 3 - Zaglavlje kolona
        o << "<Style ss:ID=\"s3\">"
             "<Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>"
             "<Font ss:FontName=\"Calibri\" ss:Size=\"9\" ss:Bold=\"1\" ss:Color=\"#93C5FD\"/>"
             "<Interior ss:Color=\"#1E3A5F\" ss:Pattern=\"Solid\"/>"
             "<Borders><Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"2\" ss:Color=\"#2563EB\"/></Borders>"
             "</Style>\n";
        // 4 - Zaglavlje lijevo
        o << "<Style ss:ID=\"s3l\">"
             "<Alignment ss:Horizontal=\"Left\" ss:Vertical=\"Center\"/>"
             "<Font ss:FontName=\"Calibri\" ss:Size=\"9\" ss:Bold=\"1\" ss:Color=\"#93C5FD\"/>"
             "<Interior ss:Color=\"#1E3A5F\" ss:Pattern=\"Solid\"/>"
             "<Borders><Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"2\" ss:Color=\"#2563EB\"/></Borders>"
             "</Style>\n";

        // Stilovi za statuse
        auto makeRowStyle = [&](const QString &id, const QString &bg, const QString &fg,
                                bool bold=false, const QString &align="Center") {
            o << "<Style ss:ID=\"" << id << "\">"
              << "<Alignment ss:Horizontal=\"" << align << "\" ss:Vertical=\"Center\"/>"
              << "<Font ss:FontName=\"Calibri\" ss:Size=\"10\""
              << (bold?" ss:Bold=\"1\"":"") << " ss:Color=\"" << fg << "\"/>"
              << "<Interior ss:Color=\"" << bg << "\" ss:Pattern=\"Solid\"/>"
              << "<Borders><Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#1E2535\"/></Borders>"
              << "</Style>\n";
        };
        // po statusu: bg, fg
        makeRowStyle("ru",  "#0F2016","#86EFAC", false,"Left");  // uredu naziv
        makeRowStyle("ruc", "#0F2016","#D0ECD8", false,"Center");
        makeRowStyle("rub", "#0F2016","#22C55E", true, "Center");// uredu status
        makeRowStyle("up",  "#261E06","#FDE68A", false,"Left");
        makeRowStyle("upc", "#261E06","#F0D8A0", false,"Center");
        makeRowStyle("upb", "#261E06","#F59E0B", true, "Center");
        makeRowStyle("kr",  "#2A1204","#FDBA74", false,"Left");
        makeRowStyle("krc", "#2A1204","#F8C8A0", false,"Center");
        makeRowStyle("krb", "#2A1204","#F97316", true, "Center");
        makeRowStyle("is",  "#280808","#FCA5A5", false,"Left");
        makeRowStyle("isc", "#280808","#F0A0A0", false,"Center");
        makeRowStyle("isb", "#280808","#EF4444", true, "Center");
        makeRowStyle("ne",  "#1E1030","#C4B5FD", false,"Left");
        makeRowStyle("nec", "#1E1030","#D4C0F8", false,"Center");
        makeRowStyle("neb", "#1E1030","#A855F7", true, "Center");
        makeRowStyle("za",  "#161A28","#94A3B8", false,"Left");
        makeRowStyle("zac", "#161A28","#7890B0", false,"Center");
        makeRowStyle("zab", "#161A28","#64748B", true, "Center");
        // Da/Ne stilovi
        makeRowStyle("gr_ru", "#0F2016","#22C55E", true,"Center");
        makeRowStyle("rd_ru", "#0F2016","#EF4444", true,"Center");
        makeRowStyle("gr_up", "#261E06","#22C55E", true,"Center");
        makeRowStyle("rd_up", "#261E06","#EF4444", true,"Center");
        makeRowStyle("gr_kr", "#2A1204","#22C55E", true,"Center");
        makeRowStyle("rd_kr", "#2A1204","#EF4444", true,"Center");
        makeRowStyle("gr_is", "#280808","#22C55E", true,"Center");
        makeRowStyle("rd_is", "#280808","#EF4444", true,"Center");
        makeRowStyle("gr_ne", "#1E1030","#22C55E", true,"Center");
        makeRowStyle("rd_ne", "#1E1030","#EF4444", true,"Center");
        makeRowStyle("gr_za", "#161A28","#22C55E", true,"Center");
        makeRowStyle("rd_za", "#161A28","#EF4444", true,"Center");

        o << "</Styles>\n";

        o << "<Worksheet ss:Name=\"" << TR("Evidencija","Evidence") << "\">\n";
        o << "<Table>\n";

        // Sirine kolona
        QVector<int> colW = {200,80,90,90,90,90,100,70,80,100,200};
        for(int w : colW) o << "<Column ss:Width=\"" << w << "\"/>\n";

        // Naslovni red
        o << "<Row ss:Height=\"32\">"
          << "<Cell ss:MergeAcross=\"10\" ss:StyleID=\"s1\">"
          << "<Data ss:Type=\"String\">VALTER  —  "
          << TR("Program za evidenciju filmova i kljuceva za bioskop  |  KC Bar",
                "Film and Key Management Program  |  KC Bar")
          << "</Data></Cell></Row>\n";

        o << "<Row ss:Height=\"20\">"
          << "<Cell ss:MergeAcross=\"10\" ss:StyleID=\"s2\">"
          << "<Data ss:Type=\"String\">"
          << TR("Generirano: ","Generated: ")
          << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm")
          << TR(QString("   |   Aktivnih: %1   Upozorenja: %2   Isteklih: %3").arg(uk).arg(upo).arg(ist),
                QString("   |   Active: %1   Warnings: %2   Expired: %3").arg(uk).arg(upo).arg(ist))
          << "</Data></Cell></Row>\n";

        // Prazni red
        o << "<Row ss:Height=\"8\"><Cell ss:StyleID=\"s2\"><Data ss:Type=\"String\"></Data></Cell></Row>\n";

        // Zaglavlje
        QStringList hdr = {
            TR("NAZIV FILMA","FILM TITLE"),
            TR("FORMAT","FORMAT"),
            TR("KLJUC","KEY"),
            TR("OD","FROM"),
            TR("DO","TO"),
            TR("TRAJANJE","DURATION"),
            TR("PREOSTALO","REMAINING"),
            TR("SKINUT","DOWNLOADED"),
            TR("INJESTOVAN","INGESTED"),
            TR("STATUS","STATUS"),
            TR("NAPOMENA","NOTE")
        };
        o << "<Row ss:Height=\"22\">";
        o << "<Cell ss:StyleID=\"s3l\"><Data ss:Type=\"String\">" << hdr[0] << "</Data></Cell>";
        for(int i=1;i<hdr.size();i++)
            o << "<Cell ss:StyleID=\"s3\"><Data ss:Type=\"String\">" << hdr[i] << "</Data></Cell>";
        o << "</Row>\n";

        // Redovi podataka
        for (const auto &fi : filmovi) {
            if (fi.uArhivi) continue;
            RowStatus st = rowStatus(fi);

            QString pfx, sfx, dfx;
            switch(st) {
                case RS_UREDU:       pfx="ru";  sfx="ruc";  dfx="gr_ru"; break;
                case RS_UPOZORENJE:  pfx="up";  sfx="upc";  dfx="gr_up"; break;
                case RS_KRITICNO:    pfx="kr";  sfx="krc";  dfx="gr_kr"; break;
                case RS_ISTEKAO:     pfx="is";  sfx="isc";  dfx="gr_is"; break;
                case RS_NEOGRANICEN: pfx="ne";  sfx="nec";  dfx="gr_ne"; break;
                default:             pfx="za";  sfx="zac";  dfx="gr_za"; break;
            }
            QString stBold = pfx + "b";
            QString rdStyle = "rd_" + pfx;

            int dana = danaDoIsteka(fi);
            QString pre;
            if (!fi.kljucOtkljucan)    pre = "—";
            else if (fi.bezOgranicenja) pre = "Neograniceno";
            else if (dana < 0)          pre = TR(QString("Istekao %1d").arg(-dana), QString("Expired %1d").arg(-dana));
            else if (dana == 0)         pre = TR("DANAS","TODAY");
            else if (dana == 1)         pre = TR("SUTRA","TOMORROW");
            else                        pre = TR(QString("%1 dana").arg(dana), QString("%1 days").arg(dana));

            QString kl = !fi.kljucOtkljucan ? TR("Zakljucan","Locked")
                       : fi.bezOgranicenja  ? TR("Neogranicen","Unlimited") : TR("Otkljucan","Unlocked");
            QString od  = (!fi.kljucOtkljucan||fi.bezOgranicenja) ? "—" : fi.datumOd.toString("dd.MM.yyyy");
            QString doo = (!fi.kljucOtkljucan||fi.bezOgranicenja) ? "—" : fi.isticaKljuca.toString("dd.MM.yyyy");
            QString tr_ = !fi.kljucOtkljucan ? "—" : fi.bezOgranicenja ? "∞"
                        : TR(QString("%1 dana").arg(trajanjeDana(fi)), QString("%1 days").arg(trajanjeDana(fi)));
            QString stStr;
            switch(st){
                case RS_UREDU:       stStr=TR("U REDU","OK");          break;
                case RS_UPOZORENJE:  stStr=TR("UPOZORENJE","WARNING");  break;
                case RS_KRITICNO:    stStr=TR("KRITICNO","CRITICAL");   break;
                case RS_ISTEKAO:     stStr=TR("ISTEKAO","EXPIRED");     break;
                case RS_NEOGRANICEN: stStr=TR("NEOGRANICEN","UNLIMITED");break;
                default:             stStr=TR("ZAKLJUCAN","LOCKED");    break;
            }

            auto cell = [&](const QString &style, const QString &val) {
                QString safe = val;
                safe.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;");
                return QString("<Cell ss:StyleID=\"%1\"><Data ss:Type=\"String\">%2</Data></Cell>").arg(style).arg(safe);
            };

            o << "<Row ss:Height=\"20\">"
              << cell(pfx,  fi.naziv)
              << cell(sfx,  fi.format)
              << cell(sfx,  kl)
              << cell(sfx,  od)
              << cell(sfx,  doo)
              << cell(sfx,  tr_)
              << cell(sfx,  pre)
              << cell(fi.filmSkinut?dfx:rdStyle,     fi.filmSkinut?TR("Da","Yes"):TR("Ne","No"))
              << cell(fi.filmInjestovan?dfx:rdStyle, fi.filmInjestovan?TR("Da","Yes"):TR("Ne","No"))
              << cell(stBold, stStr)
              << cell(sfx,  fi.napomena)
              << "</Row>\n";
        }

        o << "</Table>\n</Worksheet>\n</Workbook>\n";
        f.close();
        QMessageBox::information(this,
            TR("Excel Izvezen","Excel Exported"),
            TR(QString("Excel sacuvan:\n%1\n\nOtvorite u Microsoft Excelu — bez upozorenja!").arg(path),
               QString("Excel saved:\n%1\n\nOpen in Microsoft Excel — no warnings!").arg(path)));

    } else {
        // ── CSV ──────────────────────────────────────────────────────────
        QString path = QFileDialog::getSaveFileName(this,
            TR("Izvezi CSV","Export CSV"),
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
            + "/Valter_" + QDate::currentDate().toString("dd-MM-yyyy") + ".csv",
            "CSV (*.csv)");
        if (path.isEmpty()) return;
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream o(&f);
        o.setCodec("UTF-8");
        o << "\xEF\xBB\xBF";
        o << TR("Naziv;Format;Kljuc;Bez ogranicenja;Od;Do;Trajanje;Preostalo;Skinut;Injestovan;Arhiva;Napomena",
                "Name;Format;Key;Unlimited;From;To;Duration;Remaining;Downloaded;Ingested;Archive;Note") << "\n";
        for (const auto &fi : filmovi) {
            int d = danaDoIsteka(fi);
            QString pre = fi.bezOgranicenja ? TR("Neograniceno","Unlimited")
                : (fi.kljucOtkljucan ? QString::number(d) : "-");
            o << "\"" << fi.naziv << "\";" << fi.format << ";"
              << TR(fi.kljucOtkljucan?"Da":"Ne", fi.kljucOtkljucan?"Yes":"No") << ";"
              << TR(fi.bezOgranicenja?"Da":"Ne", fi.bezOgranicenja?"Yes":"No") << ";"
              << (fi.datumOd.isValid()?fi.datumOd.toString("dd.MM.yyyy"):"") << ";"
              << (fi.isticaKljuca.isValid()?fi.isticaKljuca.toString("dd.MM.yyyy"):"") << ";"
              << (fi.bezOgranicenja?TR("Neograniceno","Unlimited"):QString::number(trajanjeDana(fi))) << ";"
              << pre << ";"
              << TR(fi.filmSkinut?"Da":"Ne", fi.filmSkinut?"Yes":"No") << ";"
              << TR(fi.filmInjestovan?"Da":"Ne", fi.filmInjestovan?"Yes":"No") << ";"
              << TR(fi.uArhivi?"Da":"Ne", fi.uArhivi?"Yes":"No") << ";"
              << "\"" << fi.napomena << "\"\n";
        }
        f.close();
        QMessageBox::information(this,
            TR("CSV Izvezen","CSV Exported"),
            TR(QString("CSV sacuvan:\n%1").arg(path),
               QString("CSV saved:\n%1").arg(path)));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Uvezi
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::uvezi() {
    QString path = QFileDialog::getOpenFileName(this, "Uvezi podatke",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        "JSON fajlovi (*.json);;CSV fajlovi (*.csv)");
    if (path.isEmpty()) return;
    int uvezeno = 0;
    if (path.endsWith(".json", Qt::CaseInsensitive)) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) { QMessageBox::warning(this,"Greska","Ne mogu otvoriti JSON."); return; }
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll()); f.close();
        if (!doc.isArray()) { QMessageBox::warning(this,"Greska","Neispravan JSON format."); return; }
        for (const QJsonValue &v : doc.array()) {
            QJsonObject o = v.toObject();
            Film film;
            film.naziv          = o["naziv"].toString();
            film.format         = o["format"].toString();
            film.kljucOtkljucan = o["kljucOtkljucan"].toBool();
            film.bezOgranicenja = o["bezOgranicenja"].toBool();
            film.datumOd        = QDate::fromString(o["datumOd"].toString(), Qt::ISODate);
            film.isticaKljuca   = QDate::fromString(o["isticaKljuca"].toString(), Qt::ISODate);
            film.filmSkinut     = o["filmSkinut"].toBool();
            film.filmInjestovan = o["filmInjestovan"].toBool();
            film.uArhivi        = o["uArhivi"].toBool();
            film.napomena       = o["napomena"].toString();
            if (!film.naziv.isEmpty()) { filmovi.append(film); uvezeno++; }
        }
    } else {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly|QIODevice::Text)) { QMessageBox::warning(this,"Greska","Ne mogu otvoriti CSV."); return; }
        QTextStream in(&f);
        in.readLine();
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;
            QStringList cols = line.split(";");
            if (cols.size() < 9) continue;
            Film film;
            film.naziv          = cols[0].remove('"');
            film.format         = cols[1];
            film.kljucOtkljucan = (cols[2]=="Da");
            film.bezOgranicenja = (cols[3]=="Da");
            film.datumOd        = QDate::fromString(cols[4],"dd.MM.yyyy");
            film.isticaKljuca   = QDate::fromString(cols[5],"dd.MM.yyyy");
            film.filmSkinut     = (cols[8]=="Da");
            film.filmInjestovan = (cols.size()>9 && cols[9]=="Da");
            film.uArhivi        = (cols.size()>10 && cols[10]=="Da");
            film.napomena       = (cols.size()>11 ? cols[11].remove('"') : "");
            if (!film.naziv.isEmpty()) { filmovi.append(film); uvezeno++; }
        }
        f.close();
    }
    sacuvajPodatke(); popuniTabelu(); popuniArhivu();
    azurirajStatusBar(); azurirajBrojacTabova(); provjeriIstekKljuceva();
    QMessageBox::information(this,"Uvoz Zavrsen", QString("Uvezeno %1 film(ova).").arg(uvezeno));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Perzistencija
// ─────────────────────────────────────────────────────────────────────────────
QString MainWindow::dataFilePath() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/evidencija.json";
}
void MainWindow::ucitajPodatke() {
    QFile f(dataFilePath());
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll()); f.close();
    filmovi.clear();
    for (const QJsonValue &v : doc.array()) {
        QJsonObject o = v.toObject();
        Film film;
        film.naziv          = o["naziv"].toString();
        film.format         = o["format"].toString();
        film.kljucOtkljucan = o["kljucOtkljucan"].toBool();
        film.bezOgranicenja = o["bezOgranicenja"].toBool();
        film.datumOd        = QDate::fromString(o["datumOd"].toString(), Qt::ISODate);
        film.isticaKljuca   = QDate::fromString(o["isticaKljuca"].toString(), Qt::ISODate);
        film.filmSkinut     = o["filmSkinut"].toBool();
        film.filmInjestovan = o["filmInjestovan"].toBool();
        film.uArhivi        = o["uArhivi"].toBool();
        film.napomena       = o["napomena"].toString();
        filmovi.append(film);
    }
}
void MainWindow::sacuvajPodatke() {
    QJsonArray arr;
    for (const auto &f : filmovi) {
        QJsonObject o;
        o["naziv"]          = f.naziv;
        o["format"]         = f.format;
        o["kljucOtkljucan"] = f.kljucOtkljucan;
        o["bezOgranicenja"] = f.bezOgranicenja;
        o["datumOd"]        = f.datumOd.toString(Qt::ISODate);
        o["isticaKljuca"]   = f.isticaKljuca.toString(Qt::ISODate);
        o["filmSkinut"]     = f.filmSkinut;
        o["filmInjestovan"] = f.filmInjestovan;
        o["uArhivi"]        = f.uArhivi;
        o["napomena"]       = f.napomena;
        arr.append(o);
    }
    QFile f(dataFilePath());
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(arr).toJson());
}

void MainWindow::openSettings() {
    SettingsDialog dlg(this);

    if (dlg.exec() != QDialog::Accepted) return;

    // exec() je vratio - dijalog je POTPUNO zatvoren
    // Sada je sigurno primjeniti sve promjene

    // 1. Primijeni jezik
    Lang::instance().setLanguage(dlg.selectedLang());

    // 2. Primijeni temu (mijenja stylesheet)
    Theme::instance().set(dlg.selectedTheme());

    // 3. Osvježi svo sučelje
    refreshUI();
}

void MainWindow::refreshUI() {
    // Osvježi naslove i tekstove sučelja
    setWindowTitle(TR("Valter — Program za evidenciju filmova i kljuceva za bioskop",
                      "Valter — Film and Key Management Program for Cinema"));

    // Tab naslovi
    azurirajBrojacTabova();

    // Zaglavlja tabele — aktivni
    tabela->setHorizontalHeaderLabels({
        TR("NAZIV FILMA","FILM TITLE"),
        TR("FORMAT","FORMAT"),
        TR("KLJUC","KEY"),
        TR("OD","FROM"),
        TR("DO","TO"),
        TR("TRAJANJE","DURATION"),
        TR("PREOSTALO","REMAINING"),
        TR("SKINUT","DOWNLOADED"),
        TR("INJESTOVAN","INGESTED"),
        TR("STATUS","STATUS"),
        TR("NAPOMENA","NOTE")
    });
    // Zaglavlja tabele — arhiva
    tabelaArhiva->setHorizontalHeaderLabels({
        TR("NAZIV FILMA","FILM TITLE"),
        TR("FORMAT","FORMAT"),
        TR("KLJUC","KEY"),
        TR("OD","FROM"),
        TR("DO","TO"),
        TR("TRAJANJE","DURATION"),
        TR("PREOSTALO","REMAINING"),
        TR("SKINUT","DOWNLOADED"),
        TR("INJESTOVAN","INGESTED"),
        TR("STATUS","STATUS"),
        TR("NAPOMENA","NOTE")
    });

    // Toolbar dugmad
    btnDodaj->setText(TR("＋  Dodaj Film","＋  Add Film"));
    btnUredi->setText(TR("✎  Uredi","✎  Edit"));
    btnObrisi->setText(TR("✕  Obrisi","✕  Delete"));
    btnArhiviraj->setText(TR("📁  U Arhivu","📁  Archive"));
    btnOsvezi->setText(TR("↻  Osvjezi","↻  Refresh"));
    btnPDF->setText(TR("📄  PDF","📄  PDF"));
    btnCSV->setText(TR("📊  Excel/CSV","📊  Excel/CSV"));
    btnUvezi->setText(TR("📥  Uvezi","📥  Import"));
    edPretraga->setPlaceholderText(TR("Pretrazi filmove...","Search films..."));
    btnVratiIzArhive->setText(TR("↩  Vrati u aktivne","↩  Restore"));
    btnObrisiArhiva->setText(TR("✕  Trajno obrisi","✕  Delete permanently"));
    btnSettings->setToolTip(TR("Podešavanja (tema, jezik)","Settings (theme, language)"));

    // Osvježi podatke
    popuniTabelu(edPretraga->text());
    popuniArhivu(edPretraga->text());
    azurirajStatusBar();
    provjeriIstekKljuceva();
}
