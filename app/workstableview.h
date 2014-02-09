#ifndef WORKSTABLEVIEW_H
#define WORKSTABLEVIEW_H

#include <QWidget>
#include <QTableView>
#include <QSortFilterProxyModel>

class DB;

class WorksTableView : public QTableView
{
  Q_OBJECT

  public:
    explicit WorksTableView(QWidget *parent = 0);
    void setModel(QAbstractItemModel * model);
    ~WorksTableView();
  protected:
    virtual void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  public slots:
    void readSettings();
    void writeSettings();
    void selectWorkRelative(int rows);
    void emitSelected();
    void setSessionNumber(int session);
  signals:
    void workSelected(int workid);
  private:
    int mSessionNumber = 0;
};

class WorksSortFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
  public:
    WorksSortFilterProxyModel(QObject * parent = nullptr);
  protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const;
};

class SessionDisplayStyle : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QColor backgroundColor READ backgroundColorGet WRITE backgroundColorSet DESIGNABLE true)
  public:
    SessionDisplayStyle() : backgroundColor(Qt::red) { }

    QColor backgroundColorGet() const { return backgroundColor; }
    void backgroundColorSet(QColor color) { backgroundColor = color; }
  private:
    QColor backgroundColor;
};

#endif // WORKSTABLEVIEW_H
