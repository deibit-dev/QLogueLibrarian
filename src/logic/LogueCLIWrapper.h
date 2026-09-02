#ifndef LOGUECLIWRAPPER_H
#define LOGUECLIWRAPPER_H

#include <QObject>
#include <QString>
#include <QVector>

#include "model/MidiPort.h"

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
class LogueCLIWrapper : public QObject {
    Q_OBJECT
public:
    explicit LogueCLIWrapper(QObject *parent = nullptr);

    /// Run `probe -l` and parse available MIDI ports.
    void probe(const QString &cliPath);

    /// Run `load -u <unitPath> -i <inPort> -o <outPort> [-s <slot>]`
    void loadUnit(const QString &cliPath, const QString &unitPath,
                  int inPort, int outPort, int slot = -1);

signals:
    void probeFinished(QVector<qlogue::MidiPort> ports);
    void loadFinished(qlogue::LoadResult result);
    void errorOccurred(QString message);

private:
    static QVector<MidiPort> parseProbeOutput(const QString &text);
    static LoadResult        parseLoadOutput(const QString &text, int exitCode);
};

} // namespace qlogue

#endif // LOGUECLIWRAPPER_H
