#include "audiomodel.h"

#include <iostream>
using std::cout;
using std::endl;

AudioModel::AudioModel(QObject *parent) :
  QObject(parent)
{
}

void AudioModel::playerSetValueDouble(int player, QString name, double v) {
  cout << player << " name " << qPrintable(name) << v << endl;
}

void AudioModel::playerSetValueInt(int player, QString name, int v) {
  cout << player << " name " << qPrintable(name) << v << endl;
}

void AudioModel::playerSetValueBool(int player, QString name, bool v) {
  cout << player << " name " << qPrintable(name) << v << endl;
}

void AudioModel::playerTrigger(int player, QString name) {
  cout << player << " name " << qPrintable(name) << endl;
}

void AudioModel::masterSetValueDouble(QString name, double v) {
  cout << "master name " << qPrintable(name) << v << endl;
}

void AudioModel::masterSetValueInt(QString name, int v) {
  cout << "master name " << qPrintable(name) << v << endl;
}

void AudioModel::masterSetValueBool(QString name, bool v) {
  cout << "master name " << qPrintable(name) << v << endl;
}

void AudioModel::masterTrigger(QString name) {
  cout << "master name " << qPrintable(name) << endl;
}

void AudioModel::processAudio(bool doit) {
}
