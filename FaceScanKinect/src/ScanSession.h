#ifndef SCANSESSION_H
#define SCANSESSION_H

#include <QString>
#include <QDateTime>
#include <QDir>

class ScanSession {
public:
    ScanSession() {
        defaultDir = QDir("..\\..\\data\\snapshots\\");
        scanSession = defaultDir.absolutePath();
        initialized = false;
    }

    ~ScanSession() { }

    void newScanSession() {
        QDateTime now = QDateTime::currentDateTime();
        scanSession = defaultDir.absolutePath() + QDir::separator() + now.toString("yyyy_MM_dd_HH_mm_ss") + QDir::separator();
    }

    QString getCurrentScanSession() {
        if (!initialized) {
            newScanSession();
            initialized = true;
        }
        return scanSession;
    }

    void setScanSession(QString session) {
        scanSession = session;
        initialized = true;
    }

private:
    bool initialized;
    QString scanSession;
    QDir defaultDir;
};

extern ScanSession theScanSession;


#endif // SCANSESSION_H
