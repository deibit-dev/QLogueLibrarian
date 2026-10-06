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

// ── load (logue-cli -d, with rawmidi/amidi fallback for large payloads) ─────

// The `-d` debug run always ends with "Error: ... timed out" for large units
// (that is *why* we fall back to amidi). Those lines are not real failures for
// the user, so they are dropped from the log once we resend the payload.
static QString stripErrorLines(const QString &text) {
    QStringList kept;
    const QStringList lines = text.split(QLatin1Char('\n'));
    kept.reserve(lines.size());
    for (const QString &line : lines) {
        if (line.trimmed().startsWith(QLatin1String("Error:")))
            continue;
        kept << line;
    }
    return kept.join(QLatin1Char('\n'));
}

void LogueCLIWrapper::loadUnit(const QString &cliPath, const QString &unitPath,
                               int inPort, int outPort, int slot) {
    auto *proc = new QProcess(this);
    connectStartError(proc);
    connect(proc, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this, proc](int exitCode, QProcess::ExitStatus) {
        const QString stdoutText = QString::fromUtf8(proc->readAllStandardOutput());
        const QString stderrText = QString::fromUtf8(proc->readAllStandardError());
        LoadResult r = parseLoadOutput(stdoutText + stderrText, exitCode);
        r.rawOutput = stripSysexDump(stdoutText + stderrText);
        proc->deleteLater();

        // Small payloads upload fine through logue-cli itself.
        if (exitCode == 0) {
            emit loadFinished(r);
            return;
        }

        // Larger payloads exceed the ALSA sequencer message length limit and
        // fail with a timeout, but `-d` already dumped the whole outgoing
        // SysEx sequence. Resend all of it over a rawmidi port with amidi
        // (logue_load.py / upload_effect.sh workaround).
        const QStringList msgs = extractSysexMessages(stdoutText);
        if (msgs.isEmpty()) {
            emit loadFinished(r);   // nothing to resend: genuine failure
            return;
        }

        // We are going to resend the payload, so logue-cli's own timeout
        // errors are expected noise — keep them out of the log.
        r.rawOutput = stripErrorLines(r.rawOutput);

        const QString hw = detectAmidiPort();
        if (hw.isEmpty()) {
            r.success = false;
            r.rawOutput += QStringLiteral(
                "\nERROR: no rawmidi device found via `amidi -l` "
                "(install alsa-utils / check the connection)");
            emit loadFinished(r);
            return;
        }

        auto *amidi = new QProcess(this);
        connect(amidi, &QProcess::errorOccurred, this,
                [this, amidi, r](QProcess::ProcessError e) mutable {
            if (e != QProcess::FailedToStart)
                return;
            LoadResult rr = r;
            rr.success = false;
            rr.rawOutput += QStringLiteral(
                "\nERROR: failed to start amidi: %1").arg(amidi->errorString());
            emit loadFinished(rr);
            amidi->deleteLater();
        });
        connect(amidi, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
                this, [this, amidi, r, hw, n = msgs.size()](int ec, QProcess::ExitStatus) mutable {
            const QString aout = QString::fromUtf8(amidi->readAllStandardOutput())
                               + QString::fromUtf8(amidi->readAllStandardError());
            LoadResult rr = r;
            rr.success = (ec == 0);
            rr.rawOutput += QStringLiteral(
                "\n[amidi -p %1: resent %2 SysEx message(s), exit %3]")
                    .arg(hw).arg(n).arg(ec);
            if (!aout.trimmed().isEmpty())
                rr.rawOutput += QLatin1Char('\n') + aout.trimmed();
            emit loadFinished(rr);
            amidi->deleteLater();
        });

        // One amidi invocation, one -S per payload message (logue_load.py
        // sends the single post-"size:" message the same way).
        QStringList amidiArgs{QStringLiteral("-p"), hw};
        for (const QString &msg : msgs)
            amidiArgs << QStringLiteral("-S") << msg;
        amidi->start(QStringLiteral("amidi"), amidiArgs);
    });

    QStringList args{QStringLiteral("load"), QStringLiteral("-d"),
                     QStringLiteral("-u"), unitPath,
                     QStringLiteral("-i"), QString::number(inPort),
                     QStringLiteral("-o"), QString::number(outPort)};
    if (slot >= 0)
        args << QStringLiteral("-s") << QString::number(slot);

    proc->start(cliPath, args);
}

// ── SysEx extraction / rawmidi port detection ───────────────────────────────

QStringList LogueCLIWrapper::extractSysexMessages(const QString &stdoutText) {
    // Only the payload dumped AFTER the "size:" line is resent. Everything
    // before it (the '>>>'/'<<<' request/response pairs) is the device
    // handshake that logue-cli already performed; replaying it confuses the
    // device. This matches logue_load.py's extraction.
    static const QRegularExpression rx(
        QStringLiteral("^>>> \\{\\s*(.*?)\\s*\\}\\s*$"));

    bool afterSize = false;
    QStringList msgs;
    for (const QString &line : stdoutText.split(QLatin1Char('\n'))) {
        if (line.startsWith(QLatin1String("size: ")))
            afterSize = true;
        if (!afterSize)
            continue;

        const QRegularExpressionMatch m = rx.match(line);
        if (!m.hasMatch())
            continue;
        QString hex = m.captured(1);
        hex.replace(QLatin1Char(','), QLatin1Char(' '));
        hex = hex.simplified();          // "f0 42 30 ..."
        if (!hex.isEmpty())
            msgs << hex;
    }
    return msgs;
}

QString LogueCLIWrapper::stripSysexDump(const QString &text) {
    QStringList kept;
    const QStringList lines = text.split(QLatin1Char('\n'));
    kept.reserve(lines.size());
    for (const QString &line : lines) {
        const QString t = line.trimmed();
        if (t.startsWith(QLatin1String(">>> {")) ||
            t.startsWith(QLatin1String("<<< {")))
            continue;   // don't flood the log with the raw payload
        kept << line;
    }
    return kept.join(QLatin1Char('\n'));
}

QString LogueCLIWrapper::detectAmidiPort() {
    QProcess p;
    p.start(QStringLiteral("amidi"), {QStringLiteral("-l")});
    if (!p.waitForStarted(2000) || !p.waitForFinished(2000) || p.exitCode() != 0)
        return {};

    const QString out = QString::fromUtf8(p.readAllStandardOutput());
    QStringList candidates;
    QStringList nonMidi;   // the SDK SOUND port is not the physical MIDI port
    const QRegularExpression ws(QStringLiteral("\\s+"));
    for (const QString &line : out.split(QLatin1Char('\n'))) {
        if (!line.startsWith(QLatin1String("IO  ")))
            continue;
        const QStringList parts = line.split(ws, Qt::SkipEmptyParts);
        if (parts.size() < 3)
            continue;
        const QString hw = parts.at(1);
        const QString desc = parts.mid(2).join(QLatin1Char(' ')).toLower();
        const bool isLogue = desc.startsWith(QLatin1String("nts-1"))
                          || desc.startsWith(QLatin1String("minilogue xd"))
                          || desc.startsWith(QLatin1String("prologue"));
        if (!isLogue)
            continue;
        candidates << hw;
        if (!desc.contains(QLatin1String("midi")))
            nonMidi << hw;
    }

    if (nonMidi.size() == 1)   return nonMidi.first();
    if (candidates.size() == 1) return candidates.first();
    return {};   // none, or ambiguous
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
