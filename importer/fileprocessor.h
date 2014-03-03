#ifndef FILEPROCESSOR_H
#define FILEPROCESSOR_H

#include <QObject>
#include <QStringList>

class FileProcessor : public QObject
{
    Q_OBJECT
  public:
    explicit FileProcessor(QObject *parent = 0);

  signals:
    void complete();

  public slots:
    void addFiles(QStringList files);
    void process();
  private:
    QStringList mFiles;
};

#endif // FILEPROCESSOR_H
