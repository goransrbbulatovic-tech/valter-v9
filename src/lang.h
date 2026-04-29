#ifndef LANG_H
#define LANG_H
#include <QString>
#include <QMap>
#include <QSettings>

class Lang {
public:
    enum Language { SR, EN };

    static Lang& instance() { static Lang l; return l; }

    void setLanguage(Language lang) {
        m_lang = lang;
        QSettings s("KCBar", "Valter");
        s.setValue("UI/Language", lang == EN ? "en" : "sr");
    }

    Language language() const { return m_lang; }
    bool isEN() const { return m_lang == EN; }

    QString t(const QString &sr, const QString &en) const {
        return m_lang == EN ? en : sr;
    }

    void load() {
        QSettings s("KCBar", "Valter");
        m_lang = (s.value("UI/Language", "sr").toString() == "en") ? EN : SR;
    }

private:
    Lang() { load(); }
    Language m_lang = SR;
};

// Makro za lako koriscenje
#define TR(sr, en) Lang::instance().t(sr, en)

#endif
