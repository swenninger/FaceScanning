#ifndef SCANSESSION_H
#define SCANSESSION_H

#include <QString>
#include <QDateTime>

class ScanSession {
public:
    ScanSession() { scanSession = "..\\..\\data\\snapshots\\"; }
    ~ScanSession() { }

    void newScanSession() {
        QDateTime now = QDateTime::currentDateTime();

        scanSession = "..\\..\\data\\snapshots\\" + now.toString("yyyy_MM_dd_HH_mm_ss") + "\\";
    }

    QString getCurrentScanSession() {
        return scanSession;
    }

private:
    QString scanSession;
};

static ScanSession& theScanSession() {
    static ScanSession theScanSession;
    return theScanSession;
}



#endif // SCANSESSION_H
