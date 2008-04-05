/* Copyright (C) 2005-2008, Thorvald Natvig <thorvald@natvig.com>
   Copyright (C) 2008, Andreas Messer <andi@bupfen.de>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "AudioInput.h"
#include "AudioOutput.h"
#include "AudioConfigDialog.h"
#include "Global.h"
#include "NetworkConfig.h"

static ConfigWidget *AudioInputDialogNew(Settings &st) {
	return new AudioInputDialog(st);
}

static ConfigWidget *AudioOutputDialogNew(Settings &st) {
	return new AudioOutputDialog(st);
}

static ConfigRegistrar iregistrar(1000, AudioInputDialogNew);
static ConfigRegistrar oregistrar(1010, AudioOutputDialogNew);

AudioInputDialog::AudioInputDialog(Settings &st) : ConfigWidget(st) {
	QList<QString> keys;
	QString key;

	setupUi(this);

	if (AudioInputRegistrar::qmNew) {
		keys=AudioInputRegistrar::qmNew->keys();
		foreach(key, keys) {
			qcbSystem->addItem(key);
		}
	} else {
		qcbSystem->setEnabled(false);
	}

	qcbTransmit->addItem(tr("Continuous"), Settings::Continous);
	qcbTransmit->addItem(tr("Voice Activity"), Settings::VAD);
	qcbTransmit->addItem(tr("Push To Talk"), Settings::PushToTalk);
}

QString AudioInputDialog::title() const {
	return tr("Audio Input");
}

QIcon AudioInputDialog::icon() const {
	return QIcon(QLatin1String("skin:config_basic.png"));
}

void AudioInputDialog::load(const Settings &r) {
	int i;
	QList<QString> keys;

	if (AudioInputRegistrar::qmNew)
		keys=AudioInputRegistrar::qmNew->keys();
	else
		keys.clear();
	i=keys.indexOf(AudioInputRegistrar::current);
	if (i >= 0)
		loadComboBox(qcbSystem, i);

	loadComboBox(qcbTransmit, r.atTransmit);
	loadSlider(qsTransmitHold, r.iVoiceHold);
	loadSlider(qsTransmitMin, lroundf(r.fVADmin * 32767.0f));
	loadSlider(qsTransmitMax, lroundf(r.fVADmax * 32767.0f));
	loadSlider(qsFrames, r.iFramesPerPacket);
	loadSlider(qsDoublePush, r.uiDoublePush / 1000.f);

	if (r.vsVAD == Settings::Amplitude)
		qrbAmplitude->setChecked(true);
	else
		qrbSNR->setChecked(true);

	loadCheckBox(qcbPushClick, r.bPushClick);
	loadSlider(qsQuality, r.iQuality);
	loadSlider(qsComplexity, r.iComplexity);
	loadSlider(qsNoise, - r.iNoiseSuppress);
	loadSlider(qsAmp, 20000 - r.iMinLoudness);
	loadCheckBox(qcbEcho, r.bEcho);
}

void AudioInputDialog::save() const {
	s.iQuality = qsQuality->value();
	s.iNoiseSuppress = - qsNoise->value();
	s.iComplexity = qsComplexity->value();
	s.iMinLoudness = 18000 - qsAmp->value() + 2000;
	s.iVoiceHold = qsTransmitHold->value();
	s.fVADmin = qsTransmitMin->value() / 32767.0;
	s.fVADmax = qsTransmitMax->value() / 32767.0;
	s.vsVAD = qrbSNR->isChecked() ? Settings::SignalToNoise : Settings::Amplitude;
	s.iFramesPerPacket = qsFrames->value();
	s.uiDoublePush = qsDoublePush->value() * 1000;
	s.bPushClick = qcbPushClick->isChecked();
	s.atTransmit = static_cast<Settings::AudioTransmit>(qcbTransmit->currentIndex());

	s.qsAudioInput = qcbSystem->currentText();
	s.bEcho = qcbEcho->isChecked();

	if (AudioInputRegistrar::qmNew) {
		AudioInputRegistrar *air = AudioInputRegistrar::qmNew->value(qcbSystem->currentText());
		int idx = qcbDevice->currentIndex();
		if (idx > -1) {
			air->setDeviceChoice(qcbDevice->itemData(idx), s);
		}
	}
}

bool AudioInputDialog::expert(bool b) {
	qgbInterfaces->setVisible(b);
	qgbAudio->setVisible(b);
	qliComplexity->setVisible(b);
	qlComplexity->setVisible(b);
	qsComplexity->setVisible(b);
	qliFrames->setVisible(b);
	qsFrames->setVisible(b);
	qlFrames->setVisible(b);
	qswTransmit->setVisible(b);
	return true;
}

void AudioInputDialog::on_qsFrames_valueChanged(int v) {
	qlFrames->setText(tr("%1 ms").arg(v*20));
	updateBitrate();
}

void AudioInputDialog::on_qsDoublePush_valueChanged(int v) {
	if (v == 0)
		qlDoublePush->setText(tr("Off"));
	else
		qlDoublePush->setText(tr("%1 ms").arg(v));
}

void AudioInputDialog::on_qsTransmitHold_valueChanged(int v) {
	float val = v * 20;
	val = val / 1000.0f;
	qlTransmitHold->setText(tr("%1 s").arg(val, 0, 'f', 2));
}

void AudioInputDialog::on_qsQuality_valueChanged(int v) {
	qlQuality->setText(QString::number(v));
	updateBitrate();
}

void AudioInputDialog::on_qsNoise_valueChanged(int v) {
	qlNoise->setText(tr("-%1 dB").arg(v));
}

void AudioInputDialog::on_qsComplexity_valueChanged(int v) {
	qlComplexity->setText(QString::number(v));
}

void AudioInputDialog::on_qsAmp_valueChanged(int v) {
	v = 18000 - v + 2000;
	float d = 20000.0f/v;
	qlAmp->setText(QString::fromLatin1("%1").arg(d, 0, 'f', 2));
}

void AudioInputDialog::updateBitrate() {
	if (! qsQuality || ! qsFrames || ! qlBitrate)
		return;
	int q = qsQuality->value();
	int p = qsFrames->value();

	int audiorate, overhead, posrate;
	float f = q;
	void *es;

	es = speex_encoder_init(&speex_wb_mode);
	speex_encoder_ctl(es,SPEEX_SET_VBR_QUALITY, &f);
	speex_encoder_ctl(es,SPEEX_GET_BITRATE,&audiorate);
	speex_encoder_destroy(es);

	// 50 packets, in bits, IP + UDP + Crypt + type/id (Message header) + flags + seq
	overhead = 50 * 8 * (20 + 8 + 4 + 3 + 1 + 2);

	// TCP is 12 more bytes than UDP
	if (NetworkConfig::TcpModeEnabled())
		overhead += 50 * 8 * 12;

	if (g.s.bTransmitPosition)
		posrate = 12;
	else
		posrate = 0;

	posrate = posrate * 50 * 8;

	overhead = overhead / p;
	posrate = posrate / p;

	int total = audiorate + overhead + posrate;

	QPalette pal;

	if ((total / 8 > g.iMaxBandwidth) && g.uiSession) {
		pal.setColor(qlBitrate->foregroundRole(), Qt::red);
	}

	qlBitrate->setPalette(pal);

	QString v = tr("%1kbit/s (Audio %2, Position %4, Overhead %3)").arg(total / 1000.0, 0, 'f', 1).arg(audiorate / 1000.0, 0, 'f', 1).arg(overhead / 1000.0, 0, 'f', 1).arg(posrate / 1000.0, 0, 'f', 1);
	qlBitrate->setText(v);
}

void AudioInputDialog::on_qcbTransmit_currentIndexChanged(int v) {
	switch (v) {
		case 0:
			qswTransmit->setCurrentWidget(qwContinuous);
			break;
		case 1:
			qswTransmit->setCurrentWidget(qwVAD);
			break;
		case 2:
			qswTransmit->setCurrentWidget(qwPTT);
			break;
	}
}

void AudioInputDialog::on_qcbSystem_currentIndexChanged(int idx) {
	qcbDevice->clear();

	QList<audioDevice> ql;

	if (AudioInputRegistrar::qmNew) {
		AudioInputRegistrar *air = AudioInputRegistrar::qmNew->value(qcbSystem->currentText());
		ql = air->getDeviceChoices();

		foreach(audioDevice d, ql) {
			qcbDevice->addItem(d.first, d.second);
		}
		qcbEcho->setEnabled(air->canEcho(s.qsAudioOutput));
	}

	qcbDevice->setEnabled(ql.count() > 1);
}

AudioOutputDialog::AudioOutputDialog(Settings &st) : ConfigWidget(st) {
	QList<QString> keys;
	QString key;

	setupUi(this);

	if (AudioOutputRegistrar::qmNew) {
		keys=AudioOutputRegistrar::qmNew->keys();
		foreach(key, keys) {
			qcbSystem->addItem(key);
		}
	} else {
		qcbSystem->setEnabled(false);
	}

	qcbLoopback->addItem(tr("None"), Settings::None);
	qcbLoopback->addItem(tr("Local"), Settings::Local);
	qcbLoopback->addItem(tr("Server"), Settings::Server);
}

QString AudioOutputDialog::title() const {
	return tr("Audio Output");
}

QIcon AudioOutputDialog::icon() const {
	return QIcon(QLatin1String("skin:config_basic.png"));
}

void AudioOutputDialog::load(const Settings &r) {
	int i;
	QList<QString> keys;

	if (AudioOutputRegistrar::qmNew)
		keys=AudioOutputRegistrar::qmNew->keys();
	else
		keys.clear();
	i=keys.indexOf(AudioOutputRegistrar::current);
	if (i >= 0)
		loadComboBox(qcbSystem, i);

	loadSlider(qsDelay, r.iOutputDelay);
	loadSlider(qsVolume, static_cast<int>(r.fVolume * 100.0));
	loadSlider(qsJitter, r.iJitterBufferSize);
	loadComboBox(qcbLoopback, r.lmLoopMode);
	loadSlider(qsPacketDelay, static_cast<int>(r.dMaxPacketDelay));
	loadSlider(qsPacketLoss, static_cast<int>(r.dPacketLoss * 100.0));
	loadSlider(qsMinDistance, lroundf(r.fAudioMinDistance * 10));
	loadSlider(qsMaxDistance, lroundf(r.fAudioMaxDistance * 10));
	loadSlider(qsRollOff, lroundf(r.fAudioRollOff * 100));
	loadSlider(qsBloom, lround(r.fAudioBloom * 100));
	loadCheckBox(qcbHeadphones, r.bPositionalHeadphone);
	loadCheckBox(qcbPositional, r.bPositionalAudio);
}

void AudioOutputDialog::save() const {
	s.iOutputDelay = qsDelay->value();
	s.fVolume = qsVolume->value() / 100.0f;
	s.iJitterBufferSize = qsJitter->value();
	s.qsAudioOutput = qcbSystem->currentText();
	s.lmLoopMode = static_cast<Settings::LoopMode>(qcbLoopback->currentIndex());
	s.dMaxPacketDelay = qsPacketDelay->value();
	s.dPacketLoss = qsPacketLoss->value() / 100.0f;
	s.fAudioMinDistance = qsMinDistance->value() / 10.0f;
	s.fAudioMaxDistance = qsMaxDistance->value() / 10.0f;
	s.fAudioRollOff = qsRollOff->value() / 100.0f;
	s.fAudioBloom = qsBloom->value() / 100.0f;
	s.bPositionalAudio = qcbPositional->isChecked();
	s.bPositionalHeadphone = qcbHeadphones->isChecked();

	if (AudioOutputRegistrar::qmNew) {
		AudioOutputRegistrar *aor = AudioOutputRegistrar::qmNew->value(qcbSystem->currentText());
		int idx = qcbDevice->currentIndex();
		if (idx > -1) {
			aor->setDeviceChoice(qcbDevice->itemData(idx), s);
		}
	}
}

bool AudioOutputDialog::expert(bool b) {
	return b;
}

void AudioOutputDialog::on_qcbSystem_currentIndexChanged(int idx) {
	qcbDevice->clear();

	QList<audioDevice> ql;

	if (AudioOutputRegistrar::qmNew) {
		AudioOutputRegistrar *aor = AudioOutputRegistrar::qmNew->value(qcbSystem->currentText());
		ql = aor->getDeviceChoices();

		foreach(audioDevice d, ql) {
			qcbDevice->addItem(d.first, d.second);
		}
	}

	qcbDevice->setEnabled(ql.count() > 1);
}

void AudioOutputDialog::on_qsJitter_valueChanged(int v) {
	qlJitter->setText(tr("%1 ms").arg(v*20));
}

void AudioOutputDialog::on_qsVolume_valueChanged(int v) {
	qlVolume->setText(tr("%1%").arg(v));
}

void AudioOutputDialog::on_qsPacketDelay_valueChanged(int v) {
	qlPacketDelay->setText(tr("%1 ms").arg(v));
}

void AudioOutputDialog::on_qsPacketLoss_valueChanged(int v) {
	qlPacketLoss->setText(tr("%1%").arg(v));
}

void AudioOutputDialog::on_qsDelay_valueChanged(int v) {
	qlDelay->setText(tr("%1ms").arg(v*20));
}

void AudioOutputDialog::on_qcbLoopback_currentIndexChanged(int v) {
	bool ena = false;
	if (v == 1)
		ena = true;

	qsPacketDelay->setEnabled(ena);
	qlPacketDelay->setEnabled(ena);
	qsPacketLoss->setEnabled(ena);
	qlPacketLoss->setEnabled(ena);
}

void AudioOutputDialog::on_qsMinDistance_valueChanged(int v) {
	qlMinDistance->setText(tr("%1m").arg(v/10.0, 0, 'f', 1));
	if (qsMaxDistance->value() < v)
		qsMaxDistance->setValue(v);
}

void AudioOutputDialog::on_qsMaxDistance_valueChanged(int v) {
	qlMaxDistance->setText(tr("%1m").arg(v/10.0, 0, 'f', 1));
	if (qsMinDistance->value() > v)
		qsMinDistance->setValue(v);
}

void AudioOutputDialog::on_qsRollOff_valueChanged(int v) {
	qlRollOff->setText(tr("%1").arg(v/100.0, 0, 'f', 2));
}

void AudioOutputDialog::on_qsBloom_valueChanged(int v) {
	qlBloom->setText(tr("%1%").arg(v));
}

void AudioOutputDialog::on_qcbPositional_stateChanged(int v) {
	qgbVolume->setEnabled(v);
}
