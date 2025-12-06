#ifndef TOURMAPWINDOW_H
#define TOURMAPWINDOW_H

#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPoint>
#include <QVector>

class TourMapWindow : public QDialog
{
    Q_OBJECT

public:
    explicit TourMapWindow(QWidget *parent = nullptr);
    void setTourPath(const QVector<QPoint> &path);
    void highlightStep(int step);

signals:
    void stepSelected(int step);

private slots:
    void onItemClicked(QListWidgetItem *item);
    void onCloseClicked();

private:
    QListWidget *pathList;
    QVector<QPoint> tourPath;
    QLabel *titleLabel;
    QPushButton *closeButton;

    QString formatStep(int step, const QPoint &pos) const;
};

#endif // TOURMAPWINDOW_H
