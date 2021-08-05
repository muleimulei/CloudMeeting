#include "AudioInput.h"
#include "netheader.h"
#include <QAudioFormat>
#include <QDebug>
#include <QThread>

extern QUEUE_DATA<MESG> queue_send;
extern QUEUE_DATA<MESG> queue_recv;

AudioInput::AudioInput(QObject *parent)
	: QObject(parent)
{
	recvbuf = (char*)malloc(MB * 2);
	QAudioFormat format;
	//set format
	format.setSampleRate(8000);
	format.setChannelCount(1);
	format.setSampleSize(16);
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
	if (audio->state() == QAudio::ActiveState) return;
	WRITE_LOG("start collecting audio");
	inputdevice = audio->start();
	connect(inputdevice, SIGNAL(readyRead()), this, SLOT(onreadyRead()));
}

void AudioInput::stopCollect()
{
	if (audio->state() == QAudio::StoppedState) return;
	disconnect(this, SLOT(onreadyRead()));
	audio->stop();
	WRITE_LOG("stop collecting audio");
	inputdevice = nullptr;
}

void AudioInput::onreadyRead()
{
	static int num = 0, totallen  = 0;
	if (inputdevice == nullptr) return;
	int len = inputdevice->read(recvbuf + totallen, 2 * MB - totallen);
	if (num < 2)
	{
		totallen += len;
		num++;
		return;
	}
	totallen += len;
	qDebug() << "totallen = " << totallen;
	MESG* msg = (MESG*)malloc(sizeof(MESG));
	if (msg == nullptr)
	{
		qWarning() << __LINE__ << "malloc fail";
	}
	else
	{
		memset(msg, 0, sizeof(MESG));
		msg->msg_type = AUDIO_SEND;
		//Ñ¹ËõÊý¾Ý£¬×ªbase64
		QByteArray rr(recvbuf, totallen);
		QByteArray cc = qCompress(rr).toBase64();
		msg->len = cc.size();

		msg->data = (uchar*)malloc(msg->len);
		if (msg->data == nullptr)
		{
			qWarning() << "malloc mesg.data fail";
		}
		else
		{
			memset(msg->data, 0, msg->len);
			memcpy_s(msg->data, msg->len, cc.data(), cc.size());
			queue_send.push_msg(msg);
		}
	}
	totallen = 0;
	num = 0;
}

QString AudioInput::errorString()
{
	if (audio->error() == QAudio::OpenError)
	{
		return QString("AudioInput An error occurred opening the audio device").toUtf8();
	}
	else if (audio->error() == QAudio::IOError)
	{
		return QString("AudioInput An error occurred during read/write of audio device").toUtf8();
	}
	else if (audio->error() == QAudio::UnderrunError)
	{
		return QString("AudioInput Audio data is not being fed to the audio device at a fast enough rate").toUtf8();
	}
	else if (audio->error() == QAudio::FatalError)
	{
		return QString("AudioInput A non-recoverable error has occurred, the audio device is not usable at this time.");
	}
	else
	{
		return QString("AudioInput No errors have occurred").toUtf8();
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
	qDebug() << v;
	audio->setVolume(v / 100.0);
}
