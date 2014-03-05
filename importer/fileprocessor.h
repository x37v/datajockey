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

  public slots:
    void addFiles(QStringList files);
    void process();
  private:
    QStringList mFiles;
    QThreadPool * mThreadPool;
};

#endif // FILEPROCESSOR_H
