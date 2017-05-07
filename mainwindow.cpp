#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QString>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWidget>
#include <QStandardItem>
#include <QEvent>
#include <QKeyEvent>
#include <QSettings>
#include <QShortcut>
#include <stdio.h>
#include "threadtab.h"
#include "threadform.h"

//TODO decouple item model/view logic to another class
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->splitter->setStretchFactor(0,0);
    ui->splitter->setStretchFactor(1,1);
    model = new QStandardItemModel;
    model->setColumnCount(1);
    ui->treeView->setModel(model);
    ui->treeView->installEventFilter(this);
    connect(ui->treeView->selectionModel(),&QItemSelectionModel::selectionChanged,this,
            &MainWindow::onSelectionChanged);
    setShortcuts();
    //check binding sequence
    //qDebug() << QKeySequence::keyBindings(QKeySequence::PreviousChild);
}

//TODO next/prev tab on cur stackedWidget index
void MainWindow::setShortcuts(){
    QAction *nTab = new QAction(this);
    nTab->setShortcut(QKeySequence::NextChild);
    nTab->setShortcutContext(Qt::ApplicationShortcut);
    connect(nTab, &QAction::triggered, [=]{
        if(ui->treeView->selectionModel()->hasSelection())
            nextTab(ui->treeView->selectionModel()->selectedIndexes().last());
    });
    this->addAction(nTab);
    QAction *pTab = new QAction(this);
    pTab->setShortcut(QKeySequence("Ctrl+Shift+Tab"));
    pTab->setShortcutContext(Qt::ApplicationShortcut);
    connect(pTab, &QAction::triggered,[=]{
            if(ui->treeView->selectionModel()->hasSelection())
                prevTab(ui->treeView->selectionModel()->selectedIndexes().first());
        });
    this->addAction(pTab);
    QAction *del = new QAction(this);
    del->setShortcut(QKeySequence::Delete);
    connect(del, &QAction::triggered, this, &MainWindow::deleteSelected);
    this->addAction(del);
    QAction *closeTab = new QAction(this);
    closeTab->setShortcut(QKeySequence::Close);
    closeTab->setShortcutContext(Qt::ApplicationShortcut);
    connect(closeTab, &QAction::triggered, this, &MainWindow::deleteSelected);
    this->addAction(closeTab);
    QAction *navBar = new QAction(this);
    navBar->setShortcut(QKeySequence("Ctrl+l"));
    navBar->setShortcutContext(Qt::ApplicationShortcut);
    connect(navBar, &QAction::triggered, this, &MainWindow::focusBar);
    this->addAction(navBar);
    QAction *saveState = new QAction(this);
    saveState->setShortcut(QKeySequence(Qt::Key_F10));
    saveState->setShortcutContext(Qt::ApplicationShortcut);
    connect(saveState, &QAction::triggered, this, &MainWindow::saveSession);
    this->addAction(saveState);
    ui->actionSave->setShortcut(QKeySequence("Ctrl+S"));
    ui->actionSave->setShortcutContext(Qt::ApplicationShortcut);
    connect(ui->actionSave,&QAction::triggered,this,&MainWindow::saveSession);
    ui->actionReload->setShortcut(QKeySequence("Ctrl+R"));
    connect(ui->actionReload,&QAction::triggered,[=]{
        QMapIterator<int,Tab> i(tabsNew);
        while(i.hasNext()){
            Tab tab = i.next().value();
            if(tab.type == Tab::TabType::Board) ((BoardTab*)tab.TabPointer)->getPosts();
            else ((ThreadTab*)tab.TabPointer)->getPosts();
        }
    });
}

MainWindow::~MainWindow()
{
    delete model;
    delete ui;
}

void MainWindow::nextTab(QModelIndex qmi){
    if(!model->rowCount(qmi.parent())) return; //necessary?
    if(model->hasChildren(qmi) && ui->treeView->isExpanded(qmi)){
        ui->treeView->selectionModel()->setCurrentIndex(model->index(0,0,qmi),QItemSelectionModel::ClearAndSelect);
        return;
    }
    while(qmi.row() == model->rowCount(qmi.parent())-1){ //is last child and no children
        qmi = qmi.parent();
    }
    if(qmi.row()+1<model->rowCount(qmi.parent())){
        ui->treeView->selectionModel()->setCurrentIndex(model->index(qmi.row()+1,0,qmi.parent()),QItemSelectionModel::ClearAndSelect);
        return;
    }
    ui->treeView->selectionModel()->setCurrentIndex(model->index(0,0),QItemSelectionModel::ClearAndSelect);
}

void MainWindow::prevTab(QModelIndex qmi){
    if(!model->rowCount(qmi.parent())) return; //necessary?
    if(qmi.row()-1>=0){
        qmi = qmi.sibling(qmi.row()-1,0);
        while(model->hasChildren(qmi) && ui->treeView->isExpanded(qmi)){
            qmi = qmi.child(model->rowCount(qmi)-1,0);
        }
        ui->treeView->selectionModel()->setCurrentIndex(qmi,QItemSelectionModel::ClearAndSelect);
        return;
    }
    qmi = qmi.parent();
    if(qmi.row()==-1) //we're at the very first row so select last row
        ui->treeView->selectionModel()->setCurrentIndex(model->index(model->rowCount()-1,0),QItemSelectionModel::ClearAndSelect);
    else
        ui->treeView->selectionModel()->setCurrentIndex(qmi,QItemSelectionModel::ClearAndSelect);
}

void MainWindow::on_pushButton_clicked()
{
    QString searchString = ui->lineEdit->text();
    loadFromSearch(searchString,true);
}

void MainWindow::loadFromSearch(QString searchString, bool select){
    QRegularExpression re("^(?:(?:https?:\\/\\/)?boards.4chan.org)?\\/?(\\w+)(?:\\/thread)?\\/(\\d+)(?:#p\\d+)?$",QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(searchString);
    QRegularExpressionMatch match2;
    QString displayString;
    BoardTab *bt;
    if (!match.hasMatch()) {
        QRegularExpression res("^(?:(?:https?:\\/\\/)?boards.4chan.org)?\\/?(\\w+)\\/(?:catalog#s=)?(.+)?$",QRegularExpression::CaseInsensitiveOption);
        match2 = res.match(searchString);
    }
    else{
        onNewThread(this,match.captured(1),match.captured(2));
        return;
    }
    if(match2.hasMatch()){
        bt = new BoardTab(match2.captured(1),BoardType::Catalog,match2.captured(2));
        displayString = "/"+match2.captured(1)+"/"+match2.captured(2);
    }
    else{
        //return;
        bt = new BoardTab(searchString,BoardType::Index);
        displayString = "/"+searchString+"/";
    }
    qDebug().noquote() << "loading " + displayString;
    //ui->verticalLayout_3->addWidget(bt);
    ui->stackedWidget->addWidget(bt);
    //ui->stackedWidget
    Tab tab = {Tab::TabType::Board,bt,searchString};
    QStandardItem* parent1 = new QStandardItem(displayString);
    model->appendRow(parent1);
    pages++;
    parent1->setData(pages,Qt::UserRole);
    tabsNew.insert(pages,tab);
    //ui->stackedWidget->setCurrentWidget(bt);
    selectPage(pages,model);
}

void MainWindow::onNewThread(QWidget* parent, QString board, QString thread){
    qDebug().noquote()  << "loading /"+board+"/"+thread;
    ThreadTab *tt = new ThreadTab(board,thread);
    //ui->verticalLayout_3->addWidget(tt);
    ui->stackedWidget->addWidget(tt);
    Tab tab = {Tab::TabType::Thread,tt,QString("/"+board+"/"+thread)};
    QStandardItem* parent1 = new QStandardItem("/"+board+"/"+thread);
    model->appendRow(parent1);
    pages++;
    parent1->setData(pages,Qt::UserRole);
    tabsNew.insert(pages,tab);
    //ui->stackedWidget->setCurrentWidget(tt);
    //selectPage(pages,model);
}

void MainWindow::addTab(){
}

void MainWindow::on_treeView_clicked(QModelIndex index)
{
    int pageId = index.data(Qt::UserRole).toInt();
    ui->stackedWidget->setCurrentWidget((QWidget*)(tabsNew.find(pageId)->TabPointer));
    //ui->stackedWidget->setCurrentIndex(index.row());
    //show_one(index);
}

void MainWindow::show_one(QModelIndex index){
    int pageId = index.data(Qt::UserRole).toInt();
    ui->stackedWidget->setCurrentWidget((QWidget*)(tabsNew.find(pageId)->TabPointer));
}


void MainWindow::onSelectionChanged(){
    QModelIndexList list = ui->treeView->selectionModel()->selectedRows();
    if(list.size()) {
        int pageId = list.at(0).data(Qt::UserRole).toInt();
        ui->stackedWidget->setCurrentWidget((QWidget*)(tabsNew.find(pageId)->TabPointer));
    }
}

//TODO replace with regular QAction shortcuts
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        int key = keyEvent->key();
        //int mod = keyEvent->modifiers();
        //qDebug("Ate key press %d", key);
        //qDebug("Modifers %d", mod);
        if(key == 53){
            /*const QModelIndexList indexList = ui->treeView->selectionModel()->selectedRows();
            int row = indexList.at(0).row();
            qDebug() << (Tab::TabType)tabs.at(row).type;
            ((Tab::TabType)tabs.at(row).type) == Tab::TabType::Board ? ((BoardTab*)tabs.at(row).TabPointer)->updatePosts() :
                                                          ((ThreadTab*)tabs.at(row).TabPointer)->updatePosts();*/
        }
        else if(key == 16777269){
            ui->lineEdit->setFocus();
        }
        else if(key == 16777266){
            qDebug("setting focus");
            ui->treeView->setFocus();
        }
        else if(key == 16777267){
            ui->scrollAreaWidgetContents->setFocus();
        }
        else{
            return QObject::eventFilter(obj, event);
        }
        return true;
    } else {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }
}

void MainWindow::deleteSelected(){
    //TODO delete all selected; right now only deletes first one
    const QModelIndexList indexList = ui->treeView->selectionModel()->selectedRows();
    if(!indexList.size()) return;
    int pageId = indexList.at(0).data(Qt::UserRole).toInt();
    removePage(pageId,model);
    model->removeRow(indexList.at(0).row(),indexList.at(0).parent());
}

void MainWindow::removePage(int searchPage, QAbstractItemModel* model, QModelIndex parent) {
    for(int r = 0; r < model->rowCount(parent); r++) {
        QModelIndex index = model->index(r, 0, parent);
        int pageId = index.data(Qt::UserRole).toInt();
        if(pageId == searchPage || searchPage == 0){
            QWidget* tab = (QWidget*)(tabsNew.find(pageId)->TabPointer);
            ui->stackedWidget->removeWidget(tab);
            tab->deleteLater();
            tabsNew.remove(pageId);
            if( model->hasChildren(index) ) {
                removePage(0,model,index); //delete all children under match
            }
        }
        if( model->hasChildren(index) ) {
            removePage(searchPage, model, index);
        }
    }
}

void MainWindow::selectPage(int searchPage, QAbstractItemModel* model, QModelIndex parent) {
    for(int r = 0; r < model->rowCount(parent); ++r) {
        QModelIndex index = model->index(r, 0, parent);
        int pageId = index.data(Qt::UserRole).toInt();
        if(pageId == searchPage) return ui->treeView->selectionModel()->setCurrentIndex(model->index(r,0),QItemSelectionModel::ClearAndSelect);
    }
}

void MainWindow::on_lineEdit_returnPressed()
{
    on_pushButton_clicked();
    ui->treeView->setFocus();
}

void MainWindow::focusTree(){
    ui->treeView->setFocus();
}

void MainWindow::focusBar(){
    ui->lineEdit->setFocus();
    ui->lineEdit->selectAll();
}

void MainWindow::saveSession(){
    qDebug().noquote() << "Saving session.";
    QSettings settings;
    QStringList session;
    //TODO keep item order
    QMapIterator<int,Tab> i(tabsNew);
    while(i.hasNext()){
        session.append(i.next().value().searchString);
    }
    settings.setValue("session",session);
}

void MainWindow::loadSession(){
    QSettings settings;
    QStringList session = settings.value("session",QStringList()).toStringList();
    for(int i=0;i<session.length();i++){
        loadFromSearch(session.at(i),false);
    }
}
