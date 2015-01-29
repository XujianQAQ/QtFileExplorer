#include "mywindows.h"

myWindows::myWindows(QWidget *parent) :QWidget(parent)
{ 
    //Getting size of the screen
    QList<QScreen*> screenObj = QGuiApplication::screens();
    screen = screenObj.at(0);
    int sizeCol;
    screenH = screen->geometry().height();
    screenW = screen->geometry().width();
    sizeCol = 150;

    sizePreviewH = 512;
    sizePreviewW = 512;

    isShiftOn = false;
    //Globale layout
    // _____Vlayout______
    // |                |
    // |  ___HLayout___ |
    // | |            | |
    // | |   preview  | |
    // | |            | |
    // | |   file info| |
    // | |____________| |
    // |                |
    // |  column view   |
    // |________________|
    //
    layoutGlobal = new QVBoxLayout;
    layoutGlobal->setAlignment(Qt::AlignCenter);

    //Gloval preview

    preview = new imagePreview(this);

    //Preview file part

    layoutPreview = new QHBoxLayout;

    lab = new QLabel("image ici");
    lab->setMaximumHeight(screenH-sizeCol-100);
    imDef = QPixmap(":/images/test.png");
    lab->setPixmap(imDef);

    info = new fileInfo;

    //Column view part
    model = new QFileSystemModel(this);
    model->setRootPath(QDir::rootPath());

    model->setReadOnly(false);

    //Loading preferences
    loadSettings();

    columnView = new QColumnView(this);
    columnView->setMinimumHeight(sizeCol);
    columnView->setModel(model);
    //tree->setRootIndex(model->index(QDir::currentPath()));
    columnView->setCurrentIndex(model->index(lastPath));
    //columnView->setRootIndex());
    QItemSelectionModel* itSel = columnView->selectionModel();

    //Adding rename

    QPushButton *rename = new QPushButton("Rename");

    //Keyboard

    //global space shortcut
    shortcutSpace = new QShortcut(QKeySequence(Qt::Key_Space), this);
    shortcutSpace->setContext(Qt::ApplicationShortcut);

    //global enter shortcut
    shortcutEnter = new QShortcut(QKeySequence(Qt::Key_Return), this);
    //shortcutEnter->setContext(Qt::ApplicationShortcut);

    //Global Supr Shortcut
    shortcutDel = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    shortcutDel->setContext(Qt::ApplicationShortcut);

    //Qconnect
    QObject::connect(shortcutSpace,SIGNAL(activated()),this, SLOT(keyboardEvent()));
    QObject::connect(shortcutEnter,SIGNAL(activated()),this, SLOT(keyboardEnter()));
    QObject::connect(shortcutDel,SIGNAL(activated()),this, SLOT(keyboardDel()));
    //Listen to qColumnView click
    //Selection of a file
    QObject::connect(itSel,SIGNAL(currentChanged(QModelIndex,QModelIndex)),this,SLOT(clickedNew(QModelIndex,QModelIndex)));
    QObject::connect(rename,SIGNAL(clicked()),this,SLOT(rename()));

    //Adding
    layoutPreview->addWidget(lab);
    layoutPreview->addWidget(info);
    layoutGlobal->addLayout(layoutPreview);
    layoutGlobal->addWidget(columnView);
    layoutGlobal->addWidget(rename);

    //Get event even if not in front
    eater = new KeyPressEater(this);
    preview->installEventFilter(eater);

    this->setLayout(layoutGlobal);
    this->resize(1024,900);
    this->show();
}

//Update variable last*Path AND if shift is on remember all selected files
void myWindows::updatePath(QModelIndex index){
    lastFilePath = model->filePath(index);
    QFileInfo infoFile(lastFilePath);
    lastPath = infoFile.canonicalPath();
    if (isShiftOn) {
        shiftList.append(lastFilePath);
    }else{
        shiftList.clear();
        shiftList.append(lastFilePath);
    }
}

//The actionb called by the column view when the user do something
void myWindows::clickedNew(QModelIndex index,QModelIndex){
    updatePath(index);
    QString fileName = model->fileName(index);
    QFileInfo infoFile(lastFilePath);
    QString ext = fileName.split(".").back(); //We could use here QFileInfo::completeSuffix()
    int SIZE_NAME_MAX = 50;
    if (fileName.length() > SIZE_NAME_MAX) {
        info->setName(fileName.mid(0,SIZE_NAME_MAX));
    }else{
        info->setName(fileName);
    }
    if (fileName.split(".").length() == 1) {
        info->setType("Not a standard file");
    } else {
        info->setType(ext.toLower());
    }
    info->setSize(model->size(index));
    info->setResolution(0,0);
    //If it's an image we update the previews and the informations
    if (infoFile.isFile() && (ext.toLower() == "jpg" || ext.toLower() == "jpeg" || ext.toLower() == "png")) {
        lastImagePath = QString(lastFilePath);
        QPixmap imtmp(lastFilePath);
        QPixmap imtmp2 = imtmp.scaledToHeight(sizePreviewH, Qt::SmoothTransformation);
        if (imtmp2.width() > sizePreviewW) {
            lab->setPixmap(imtmp2.copy(0,0,sizePreviewW,sizePreviewH));
        }else{
            lab->setPixmap(imtmp2);
        }
        info->setResolution(imtmp.width(),imtmp.height());
        preview->updateImage(imtmp);
    }else{
        //else we show the default image
        lab->setPixmap(imDef);
    }
}

//Function to watch the global shortcut SPACE that is for showing preview
void myWindows::keyboardEvent(){
    //qDebug() << "SPACE ";
    if (preview->showing) {
        preview->hidePreview();
    } else {
        if (lastImagePath != NULL) {
            preview->showImage(lastImagePath);
            preview->activateWindow();
        }
    }
}

//Function to watch the global shortcut SPACE that is for opening the file with default app
void myWindows::keyboardEnter(){
    //qDebug() << "ENTER ";
    QDesktopServices::openUrl(QUrl::fromLocalFile(lastFilePath));
}

//Debug funtion to show all keyboard event
void myWindows::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Shift) {
        //qDebug() << "Key Shift";
        isShiftOn = true;
    }
}

void myWindows::rename(){
    bool ok;
    QString text = QInputDialog::getText(this, tr("Renamming"), tr("base name:"), QLineEdit::Normal,"", &ok);
    int num = 0;
    if (ok){
        for (int var = 0; var < shiftList.length(); ++var) {
           _rename(shiftList.at(var),text,&num);
        }
    }
}

void myWindows::_rename(QString path, QString newName,int *num){
    QFileInfo tmp(path);
    if (tmp.isFile()) {
        QFile file(path);
        QString newConstructedName = tmp.canonicalPath()+QDir::separator()+newName;
        //If the name if something XXX-01 else 01
        if (newName != "") {
            newConstructedName += "-";
        }
        if (*num >= 10) {
            if (*num < 100) {
                newConstructedName += QString("0");
            }
        } else {
            newConstructedName += QString("00");
        }
         newConstructedName += QString::number(*num);
        //if the file had an extension we keep it else nothing
        // prev.jpg -> XXX-01.jpg
        if (tmp.completeSuffix() != "") {
            newConstructedName += "."+tmp.completeSuffix();
        }
        file.rename(newConstructedName);
        *num = *num + 1;
    } else if (tmp.isDir()){
        //If we have a dir we get folders and files inside and try to rename them
        QDir fold(path);
        QStringList elmts = fold.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
        for (int var = 0; var < elmts.length(); ++var) {
            _rename(path+QDir::separator()+elmts.at(var), newName ,num);
        }
    }
}

void myWindows::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Shift) {
        //qDebug() <<"You Release Key " <<event->text();
        isShiftOn = false;
    }
}

void myWindows::loadSettings(){
    QSettings settings("IntCorpLightAssociation", "FileViewer");
    lastPath = settings.value("lastPath").toString();
}
void myWindows::saveSettings(){
    QSettings settings("IntCorpLightAssociation", "FileViewer");
    settings.setValue("lastPath", lastPath);
}

void myWindows::keyboardDel(){
    QMessageBox box;
    box.setText("Selected files/folders will be eternally deleted !!");
    box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    box.setWindowFlags(Qt::WindowStaysOnTopHint);
    box.setDefaultButton(QMessageBox::Ok);
    int ret = box.exec();
    if (ret == QMessageBox::Ok) {
        //qDebug() << "BIM ";
        for (int i = 0; i < shiftList.length(); ++i) {
            //qDebug() << "DELETE" << shiftList.at(i);
            QFileInfo tmp(shiftList.at(i));
            if (tmp.isFile()) {
                QFile file(shiftList.at(i));
                if (!file.remove()) {
                    qDebug()<<"File not deleted: "<<file.fileName();
                }
            } else {
                QDir folder(shiftList.at(i));
                if (!folder.removeRecursively()) {
                    qDebug()<<"Not all was deleted: "<<folder.absolutePath();
                }
            }
        }
        //qDebug() << "";
    }
}

//To add coloration to folders/files
//http://stackoverflow.com/questions/1397484/custom-text-color-for-certain-indexes-in-qtreeview

//When the app is closed we saved what is necessary to save
void myWindows::closeEvent(QCloseEvent*){
    saveSettings();
}

myWindows::~myWindows(){
    delete eater;
}
