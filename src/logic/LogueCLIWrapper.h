#ifndef LOGUECLIWRAPPER_H
#define LOGUECLIWRAPPER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include "model/MidiPort.h"

class QProcess;

namespace qlogue {

/// Result of a `logue-cli load` invocation.
struct LoadResult {
    bool    success = false;
    QString platform;           // "minilogue xd"
    QString module;             // "Oscillator"
    QString rawOutput;          // full stderr+stdout for the log view
};

/// Thin stateless wrapper around the logue-cli binary.
/// All heavy work runs via QProcess – signals report completion.
///
/// Uploads run `logue-cli load -d`, which prints the SysEx payload it is
/// trying to send. Small payloads upload directly through logue-cli; payloads
/// above the ALSA sequencer message limit (~2400 B) fail with a timeout there,
/// so the printed SysEx is resent over a rawmidi port with `amidi`
/// (reimplementation of the logue_load.py / upload_effect.sh workaround).
class LogueCLIWrapper : public QObject {
    Q_OBJECT
public:
    explicit LogueCLIWrapper(QObject *parent = nullptr);

    /// Run `probe -l` and parse available MIDI ports.
    void probe(const QString &cliPath);

    /// Run `load -d -u <unitPath> -i <inPort> -o <outPort> [-s <slot>]`,
    /// falling back to a rawmidi (amidi) resend for large payloads.
    void loadUnit(const QString &cliPath, const QString &unitPath,
                  int inPort, int outPort, int slot = -1);

signals:
    void probeFinished(QVector<qlogue::MidiPort> ports);
    void loadFinished(qlogue::LoadResult result);
    void errorOccurred(QString message);

private:
    /// When the binary cannot be started (e.g. wrong cliPath) QProcess emits
    /// errorOccurred(FailedToStart) but NOT finished. Without handling this
    /// the UI would wait forever for a result, so we surface it as
    /// errorOccurred().
    void connectStartError(QProcess *proc);

    static QVector<MidiPort> parseProbeOutput(const QString &text);
    static LoadResult        parseLoadOutput(const QString &text, int exitCode);

    /// Extract the SysEx payload that `logue-cli load -d` dumped *after* its
    /// "size:" line. The `>>>`/`<<<` pairs before it are the device handshake
    /// and must NOT be replayed; only the payload is resent over rawmidi
    /// (this mirrors logue_load.py exactly).
    static QStringList extractSysexMessages(const QString &stdoutText);

    /// Drop the dump lines (`>>> { ... }` / `<<< { ... }`) so the log view is
    /// not flooded with thousands of hex bytes.
    static QString stripSysexDump(const QString &text);

    /// Detect a suitable rawmidi port via `amidi -l` (empty if none/ambiguous).
    static QString detectAmidiPort();
};

} // namespace qlogue

#endif // LOGUECLIWRAPPER_H
