#pragma once

#include <QObject>
#include <QThread>
#include <QAudioOutput>
#include <QMutex>
class AudioOutput : public QThread
{
	Q_OBJECT
private:
	QAudioOutput* audio;
	QIODevice* outputdevice;
	
	volatile bool is_canRun;
	QMutex m_lock;
	void run();
public:
	AudioOutput(QObject *parent = 0);
	~AudioOutput();
	void stopImmediately();
private slots:
	void handleStateChanged(QAudio::State);
	void setVolumn(int);
	void clearQueue();
};
