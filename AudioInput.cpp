#include "AudioInput.h"
#include "netheader.h"
#include <QAudioFormat>
#include <QDebug>
#include <QThread>

extern QUEUE_SEND queue_send;
extern QUEUE_RECV queue_recv;

AudioInput::AudioInput(QObject *parent)
	: QObject(parent)
{
	recvbuf = (char*)malloc(MB * 2);
	QAudioFormat format;
	//set format
	format.setSampleRate(8000);
	format.setChannelCount(1);
	format.setSampleSize(8);
	format.setCodec("audio/pcm");
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setSampleType(QAudioFormat::UnSignedInt);

	QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
	if (!info.isFormatSupported(format))
	{
		qWarning() << "Default format not supported, trying to use the nearest.";
		format = info.nearestFormat(format);
	}
	audio = new QAudioInput(format, this);
	connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));

}

AudioInput::~AudioInput()
{
	delete audio;
}

void AudioInput::startCollect()
{
	inputdevice = audio->start();
	connect(inputdevice, SIGNAL(readyRead()), this, SLOT(onreadyRead()));
}
void AudioInput::stopCollect()
{
	disconnect(this, SLOT(onreadyRead()));
	audio->stop();
}
void AudioInput::onreadyRead()
{
	static int num = 0, totallen  = 0;
	int len = inputdevice->read(recvbuf + totallen, 2 * MB - totallen);
	if (num < 10)
	{
		totallen += len;
		num++;
		return;
	}
	totallen = 0;
	num = 0;
	MESG* msg = (MESG*)malloc(sizeof(MESG));
	if (msg == nullptr)
	{
		qWarning() << __LINE__ << "malloc fail";
	}
	else
	{
		msg->msg_type = AUDIO_SEND;
		msg->data = (uchar*)malloc(len+1);
		if (msg->data == nullptr)
		{
			qWarning() << "malloc mesg.data fail";
		}
		else
		{
			memset(msg->data, 0, len + 1);
			memcpy_s(msg->data, len + 1, recvbuf, len);
			queue_recv.push_msg(msg);
		}
	}
}

QString AudioInput::errorString()
{
	if (audio->error() == QAudio::OpenError)
	{
		return QString("An error occurred opening the audio device").toUtf8();
	}
	else if (audio->error() == QAudio::IOError)
	{
		return QString("An error occurred during read/write of audio device").toUtf8();
	}
	else if (audio->error() == QAudio::UnderrunError)
	{
		return QString("Audio data is not being fed to the audio device at a fast enough rate").toUtf8();
	}
	else if (audio->error() == QAudio::FatalError)
	{
		return QString("A non-recoverable error has occurred, the audio device is not usable at this time.");
	}
	else
	{
		return QString("No errors have occurred").toUtf8();
	}
}


void AudioInput::handleStateChanged(QAudio::State newState)
{
	switch (newState)
	{
		case QAudio::StoppedState:
			if (audio->error() != QAudio::NoError)
			{
				stopCollect();
				emit audioinputerror(errorString());
			}
			else
			{
				qWarning() << "stop recording";
			}
			break;
		case QAudio::ActiveState:
			//start recording
			qWarning() << "start recording";
			break;
		default:
			//
			break;
	}
}
void AudioInput::setVolumn(int v)
{
	audio->setVolume(v / 100.0);
}
