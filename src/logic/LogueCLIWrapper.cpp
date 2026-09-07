#include "LogueCLIWrapper.h"

#include <QProcess>
#include <QRegularExpression>

namespace qlogue {

LogueCLIWrapper::LogueCLIWrapper(QObject *parent)
    : QObject(parent) {}

void LogueCLIWrapper::connectStartError(QProcess *proc) {
    connect(proc, qOverload<QProcess::ProcessError>(&QProcess::errorOccurred),
            this, [this, proc](QProcess::ProcessError err) {
        if (err != QProcess::FailedToStart)
            return;   // crashes/timeouts surface via finished()/exit code
        emit errorOccurred(
            QStringLiteral("Failed to start %1: %2")
                .arg(proc->program(), proc->errorString()));
        proc->deleteLater();
    });
}

// ── probe -l ────────────────────────────────────────────────────────────────

void LogueCLIWrapper::probe(const QString &cliPath) {
    auto *proc = new QProcess(this);
    connectStartError(proc);
    connect(proc, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this, proc](int exitCode, QProcess::ExitStatus) {
        auto out = QString::fromUtf8(proc->readAllStandardOutput());
        if (exitCode != 0) {
            emit errorOccurred(
                QStringLiteral("probe failed (exit %1): %2")
                    .arg(exitCode)
                    .arg(QString::fromUtf8(proc->readAllStandardError())));
        } else {
            emit probeFinished(parseProbeOutput(out));
        }
        proc->deleteLater();
    });
    proc->start(cliPath, {QStringLiteral("probe"), QStringLiteral("-l")});
}

/*  Expected output:
 *    Available MIDI inputs:
 *      in  0: Midi Through:Midi Through Port-0 14:0
 *      in  1: minilogue xd:minilogue xd _ MIDI OUT 28:0
 *      in  2: minilogue xd:minilogue xd _ SOUND 28:1
 *
 *    Available MIDI ouputs:            (note: typo in original tool)
 *      out 0: Midi Through:Midi Through Port-0 14:0
 *      ...
 */
QVector<MidiPort> LogueCLIWrapper::parseProbeOutput(const QString &text) {
    QVector<MidiPort> ports;
    // matches "in  0: <name>" or "out 0: <name>"
    static const QRegularExpression rx(
        R"(^\s*(in|out)\s+(\d+):\s+(.+)$)",
        QRegularExpression::MultilineOption);

    auto it = rx.globalMatch(text);
    while (it.hasNext()) {
        auto m = it.next();
        MidiPort p;
        p.direction = (m.captured(1) == QLatin1String("in"))
                          ? MidiPort::In : MidiPort::Out;
        p.index     = m.captured(2).toInt();
        p.name      = m.captured(3).trimmed();
        ports.append(p);
    }
    return ports;
}

// ── load ────────────────────────────────────────────────────────────────────

void LogueCLIWrapper::loadUnit(const QString &cliPath, const QString &unitPath,
                               int inPort, int outPort, int slot) {
    auto *proc = new QProcess(this);
    connectStartError(proc);
    connect(proc, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this, proc](int exitCode, QProcess::ExitStatus) {
        auto combined = QString::fromUtf8(proc->readAllStandardOutput())
                      + QString::fromUtf8(proc->readAllStandardError());
        emit loadFinished(parseLoadOutput(combined, exitCode));
        proc->deleteLater();
    });

    QStringList args{QStringLiteral("load"),
                     QStringLiteral("-u"), unitPath,
                     QStringLiteral("-i"), QString::number(inPort),
                     QStringLiteral("-o"), QString::number(outPort)};
    if (slot >= 0)
        args << QStringLiteral("-s") << QString::number(slot);

    proc->start(cliPath, args);
}

/*  Expected output (success):
 *    > Parsing minilogue xd unit archive
 *    > Parsing manifest
 *    > Parsing unit binary payload
 *    > Handshaking...
 *    > Target platform: "minilogue xd"
 *    > Target module: "Oscillator"
 *    > No slot specified, using first available.
 *    size: c84 crc32: 1a39db33
 *
 *  We look for "Target platform:" and "Target module:" and a crc32 line as
 *  evidence of success.
 */
LoadResult LogueCLIWrapper::parseLoadOutput(const QString &text, int exitCode) {
    LoadResult r;
    r.rawOutput = text;

    static const QRegularExpression rxPlatform(
        R"(Target platform:\s*\"([^\"]+)\")");
    static const QRegularExpression rxModule(
        R"(Target module:\s*\"([^\"]+)\")");
    static const QRegularExpression rxCrc(
        R"(crc32:\s*[0-9a-fA-F]+)");

    auto mp = rxPlatform.match(text);
    if (mp.hasMatch()) r.platform = mp.captured(1);

    auto mm = rxModule.match(text);
    if (mm.hasMatch()) r.module = mm.captured(1);

    r.success = (exitCode == 0) && rxCrc.match(text).hasMatch();
    return r;
}

} // namespace qlogue
