#ifndef SCANSESSION_H
#define SCANSESSION_H

#include <QString>
#include <QDateTime>

class ScanSession {
public:
    ScanSession() {
        scanSession = "..\\..\\data\\snapshots\\";
        initialized = false;
    }

    ~ScanSession() { }

    void newScanSession() {
        QDateTime now = QDateTime::currentDateTime();

        scanSession = "..\\..\\data\\snapshots\\" + now.toString("yyyy_MM_dd_HH_mm_ss") + "\\";
    }

    QString getCurrentScanSession() {
        if (!initialized) {
            newScanSession();
            initialized = true;
        }
        return scanSession;
    }

    QString setScanSession(QString session) {
        scanSession = session;
    }

private:
    bool initialized;
    QString scanSession;
};

extern ScanSession theScanSession;


#endif // SCANSESSION_H
