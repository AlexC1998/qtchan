#ifndef THREADTABHELPER_H
#define THREADTABHELPER_H
#include <QJsonArray>
#include <QNetworkReply>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QImage>
#include <QMetaObject>
#include "threadform.h"

class ThreadTabHelper : public QObject
{
    Q_OBJECT
public:
    ThreadTabHelper(QString board, QString thread, QWidget* parent);
    QString board;
    QString thread;
    QMap<QString,ThreadForm*> tfMap;
    QJsonArray posts;
    QJsonObject p;
    QFutureWatcher<QImage>* imageScaler;
    bool abort = false;
    void startUp();
    static void writeJson(QString &board, QString &thread, QByteArray &rep);

private:
    QString threadUrl;
    QNetworkReply *reply;
    QNetworkRequest request;
    QWidget* parent;
    QMetaObject::Connection connectionPost;

public slots:
    void loadPosts();
    void getPosts();

signals:
    void postsLoaded(QJsonArray &posts);
    void newTF(ThreadForm* tf);
    void windowTitle(QString windowTitle);
    void setReplies(ThreadForm* tf);
    void addStretch();
};

#endif // THREADTABHELPER_H
