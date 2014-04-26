#ifndef FILEPROCESSOR_H
#define FILEPROCESSOR_H

#include <QObject>
#include <QStringList>
#include <QHash>
#include <QVariant>
#include <QThreadPool>

class FileProcessor : public QObject
{
  Q_OBJECT
  public:
    explicit FileProcessor(QObject *parent = 0);

  signals:
    void complete();
    void fileCreated(QString audioFilePath, QString annotationFilePath, QHash<QString, QVariant> tagData);
    void fileFailed(QString audioFilePath, QString message);

  public slots:
    void addFiles(QStringList files);
    void process();

    void reportFileCreated(QString audioFilePath, QString annotationFilePath, QHash<QString, QVariant> tagData);
    void reportFileFailed(QString audioFilePath, QString message);
  private:
    QStringList mFiles;
    QThreadPool * mThreadPool;
};

#endif // FILEPROCESSOR_H
