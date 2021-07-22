#include "AudioOutput.h"
#include <QMutexLocker>
#include "netheader.h"
#include <QDebug>

#ifndef FRAME_LEN_500MS
#define FRAME_LEN_500MS 8000
#endif
extern QUEUE_RECV audio_recv; //接收队列(音频)

AudioOutput::AudioOutput(QObject *parent)
	: QThread(parent)
{
	QAudioFormat format;
	format.setSampleRate(8000);
	format.setChannelCount(1);
	format.setSampleSize(16);
	format.setCodec("audio/pcm");
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setSampleType(QAudioFormat::UnSignedInt);

	QAudioDeviceInfo info = QAudioDeviceInfo::defaultOutputDevice();
	if (!info.isFormatSupported(format))
	{
		qWarning() << "Raw audio format not supported by backend, cannot play audio.";
		return;
	}
	audio = new QAudioOutput(format, this);
	connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State) ));
//	connect(this, SIGNAL(finished()), this, SLOT(clearQueue()));
}

AudioOutput::~AudioOutput()
{
	delete audio;
}
void AudioOutput::handleStateChanged(QAudio::State state)
{
	switch (state)
	{
		case QAudio::ActiveState:
			break;
		case QAudio::SuspendedState:
			break;
		case QAudio::StoppedState:
			if (audio->error() != QAudio::NoError)
			{
				qDebug() << "out audio error" << audio->error();
			}
			break;
		case QAudio::IdleState:
			break;
		case QAudio::InterruptedState:
			break;
		default:
			break;
	}
}

void AudioOutput::run()
{
	outputdevice = audio->start();
	is_canRun = true;
	QByteArray m_pcmDataBuffer;
	for (;;)
	{
		{
			QMutexLocker lock(&m_lock);
			if (is_canRun == false)
			{
				audio->stop();
				return;
			}
		}
		MESG* msg = audio_recv.pop_msg();
		if (msg == NULL) continue;
		m_pcmDataBuffer.append((char *)msg->data, msg->len);

		if (m_pcmDataBuffer.size() >= FRAME_LEN_500MS)
		{
			//写入音频数据
			qint64 ret =  outputdevice->write(m_pcmDataBuffer.data(), FRAME_LEN_500MS);
			if (ret < 0)
			{
				qDebug() << outputdevice->errorString();
				return;
			}
			else
			{
				m_pcmDataBuffer = m_pcmDataBuffer.right(m_pcmDataBuffer.size() - ret);
			}
		}
		if (msg->data) free(msg->data);
		if (msg) free(msg);
	}
}
void AudioOutput::stopImmediately()
{
	QMutexLocker lock(&m_lock);
	is_canRun = false;
}


void AudioOutput::setVolumn(int val)
{
	audio->setVolume(val / 100.0);
}

void AudioOutput::clearQueue()
{
	qDebug() << "audio recv clear";
	audio_recv.clear();
}
