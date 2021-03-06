#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    this->setWindowTitle("Bootable Drive Maker For Mac");
    hasStarted = false;
    process = new QProcess();
    connect(process,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(done(int,QProcess::ExitStatus)));
    connect(process,SIGNAL(finished(int,QProcess::ExitStatus)),&guihandler,SLOT(done(int,QProcess::ExitStatus)));
    connect(&guihandler,SIGNAL(setLineLog(QString)),this,SLOT(setLineLog(QString)));
    connect(&guihandler,SIGNAL(setBigLog(QString)),this,SLOT(setBigLog(QString)));
    connect(&guihandler,SIGNAL(autoScroll()),this,SLOT(autoScroll()));
    osPath = "";
    QProcess removeFiles;
    removeFiles.start("rm devID.txt currStep.txt cmdOut.txt path.txt file.iso file.img.dmg file.img");
    removeFiles.waitForFinished();
    QFile findDevs("findDevs.sh");
    if (findDevs.exists()) findDevs.resize(0);
    if (findDevs.open(QIODevice::WriteOnly)) {
        QTextStream out(&findDevs);
        out << "diskutil list | grep /dev/disk";
        findDevs.close();
    }
    else {
        QMessageBox msgBox;
        msgBox.setText("ERROR: Could Not Write To findDevs.sh\nThis Application Might Not Be In /Applications?");
        msgBox.exec();
        ui->plainTextEdit->appendPlainText("ERROR: Could Not Write To findDevs.sh\nThis Application Might Not Be In /Applications?");
        setEnabled(false);
    }
    on_refreshDevs_clicked();
}
MainWindow::~MainWindow() {
    delete ui;
    process->kill();
    delete process;
}
void MainWindow::setLineLog(QString str) { ui->log->setText(str); }
void MainWindow::setBigLog(QString str) {
    ui->plainTextEdit->clear();
    ui->plainTextEdit->appendPlainText(str);
}
void MainWindow::done(int exitCode, QProcess::ExitStatus exitStatus) {
    ui->startStop->setText("Quit");
    if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
        ui->plainTextEdit->appendPlainText("\n\nAn Error Has Occured. Error Code: " + QString::number(exitCode));
        ui->plainTextEdit->appendPlainText("This Application Might Not Be In /Applications?");
    }
    else {
        ui->log->setText("Process Finished.");
        ui->plainTextEdit->appendPlainText("\nProcess Finished.");
    }
}
void MainWindow::on_openISO_clicked() {
    QString filePath = QFileDialog::getOpenFileName(this,tr("Select An Operating System File"),QDir::homePath() + "/Downloads","ISO (*.iso)");
    if (filePath != "") osPath = filePath;
    ui->pathReadOut->setText(osPath);
}
void MainWindow::on_openIMG_clicked() {
    QString filePath = QFileDialog::getOpenFileName(this,tr("Select An Operating System File"),QDir::homePath() + "/Downloads","IMG (*.img)");
    if (filePath != "") osPath = filePath;
    ui->pathReadOut->setText(osPath);
}
void MainWindow::on_openDMG_clicked() {
    QString filePath = QFileDialog::getOpenFileName(this,tr("Select An Operating System File"),QDir::homePath() + "/Downloads","DMG (*.dmg)");
    if (filePath != "") osPath = filePath;
    ui->pathReadOut->setText(osPath);
}
void MainWindow::on_startStop_clicked() {
    //Run starting checks
    if (hasStarted) {
        close();
        return;
    }
    if (ui->devID->currentText().contains("ERROR")) {
        ui->plainTextEdit->appendPlainText("Please Put A USB Drive Into The Computer And Refresh The Device List.");
        ui->plainTextEdit->appendPlainText("If You Got An Error Saying Something Is Not Right, Do Not Continue, And Contact The Developer.");
        return;
    }
    if (osPath == "") {
        ui->plainTextEdit->appendPlainText("You need to select a file first.");
        return;
    }
    if (ui->devID->currentText().contains("internal")) {
        QMessageBox msgBox;
        msgBox.setText("WARNING!\n"
                       "You have selected an internal device for use.\n"
                       "To abort, click cancel in the authorization prompt after closing this message.\n"
                       "To continue, just close this message.");
        msgBox.exec();
    }
    //Disable GUI
    hasStarted = true;
    ui->log->setText("Starting...");
    ui->startStop->setText("Cancel");
    ui->devID->setEnabled(false);
    ui->openISO->setEnabled(false);
    ui->openIMG->setEnabled(false);
    ui->openDMG->setEnabled(false);
    ui->allowNonExtern->setEnabled(false);
    ui->refreshDevs->setEnabled(false);
    ui->pathReadOut->setEnabled(false);
    ui->label->setEnabled(false);
    ui->label_3->setEnabled(false);
    ui->label_4->setEnabled(false);
    //Print osPath to path.txt
    QFile location("path.txt");
    if (location.open(QIODevice::WriteOnly)) {
        QTextStream(&location) << osPath;
        location.close();
    }
    else {
        ui->log->setText("ERROR While Passing File Path");
        ui->plainTextEdit->appendPlainText("\nAn error has occured while passing the file path to the script.");
        ui->plainTextEdit->appendPlainText("This Application Might Not Be In /Applications?");
        return;
    }
    //Print rdisk device to devID.txt
    QFile dev("devID.txt");
    if (dev.open(QIODevice::WriteOnly)) {
        QTextStream(&dev) << "/dev/rdisk" << devIDToInt();
        dev.close();
    }
    else {
        ui->log->setText("ERROR While Passing The Device ID");
        ui->plainTextEdit->appendPlainText("\nAn error has occured while passing the Device ID to the script.");
        ui->plainTextEdit->appendPlainText("This Application Might Not Be In /Applications?");
    }
    //Start script
    process->start("osascript", QStringList() << "-e" << "do shell script \"sh run.sh\" with administrator privileges");
    //Start guihandler
    guihandler.start();
}
void MainWindow::autoScroll() {
    QTextCursor c = ui->plainTextEdit->textCursor();
    c.movePosition(QTextCursor::End);
    ui->plainTextEdit->setTextCursor(c);
}
void MainWindow::on_refreshDevs_clicked() {
    QProcess getDevs;
    getDevs.start("sh findDevs.sh");
    getDevs.waitForFinished();
    QByteArray output = getDevs.readAll();
    QString allDevs = QTextStream(&output).readAll();
    getDevs.close();
    if (allDevs.length() < 3) {
        ui->plainTextEdit->clear();
        ui->plainTextEdit->appendPlainText("\nCould Not Find Any Connected Devices.");
        ui->plainTextEdit->appendPlainText("Something Is Not Right. Cannot Continue.");
        ui->devID->clear();
        ui->devID->addItem("ERROR: Could Not Find Devices. See Below.");
        return;
    }
    QStringList devs;
    QString currLine = "";
    for (int i = 0; i < allDevs.length(); ++i) {
        QString c = allDevs.at(i);
        if (c != "\n") {
            if (c != ":") currLine += c;
        }
        else {
            devs.append(currLine);
            currLine = "";
        }
    }
    if (currLine != "" && currLine != " ") devs.append(currLine);
    if (allDevs.contains(":")) {
        if (!ui->allowNonExtern->isChecked()) {
            for (int i = 0; i < devs.length(); ++i) {
                if (!devs.at(i).contains("external")) {
                    devs.removeAt(i);
                    --i;
                }
            }
            if (devs.isEmpty()) devs << "ERROR: No External Devices Found.";
        }
        else if (devs.isEmpty()) devs << "ERROR: Device Parsing Error.";
    }
    else {
        ui->allowNonExtern->setEnabled(false);
        if (devs.isEmpty()) devs << "ERROR: Failed To Read Devices.";
    }
    ui->devID->clear();
    ui->devID->addItems(devs);
}
QString MainWindow::devIDToInt() {
    QString newStr = "";
    for (int i = 0; i < ui->devID->currentText().length(); ++i) {
        QString curr = ui->devID->currentText().at(i);
        if (curr >= "0" && curr <= "9") newStr += curr;
    }
    return newStr;
}
void MainWindow::on_allowNonExtern_clicked() { on_refreshDevs_clicked(); }
void MainWindow::on_actionSelectISO_triggered() { if(!hasStarted) on_openISO_clicked(); }
void MainWindow::on_actionSelectIMG_triggered() { if(!hasStarted) on_openIMG_clicked(); }
void MainWindow::on_actionSelectDMG_triggered() { if(!hasStarted) on_openDMG_clicked(); }
void MainWindow::on_actionContact_triggered() { QMessageBox::about(this,"Contact","Email: contact@etcg.pw\nWebsite: http://www.etcg.pw"); }
void MainWindow::on_actionCopyright_triggered() {
    QFile copyNotice("../Resources/copyrightNotice.txt"),
          copy("../Resources/COPYING"),
          copyLesser("../Resources/COPYING.LESSER");
    if (copyNotice.open(QIODevice::ReadOnly) && copy.open(QIODevice::ReadOnly)
        && copyLesser.open(QIODevice::ReadOnly)) {
        QTextStream inCopyNotice(&copyNotice), inCopy(&copy), inCopyLesser(&copyLesser);
        QDialog *dialog = new QDialog;
        QLabel *lb1 = new QLabel, *lb2 = new QLabel, *lb3 = new QLabel;
        lb1->setText("Copyright Notice:");
        lb2->setText("GPL License:");
        lb3->setText("LGPL License:");
        QList<QPlainTextEdit*> textHolders;
        for (int i = 0; i < 3; ++i) {
            QString textToAdd = "";
            switch (i) {
            case 0:
                textToAdd = inCopyNotice.readAll();
                break;
            case 1:
                textToAdd = inCopy.readAll();
                break;
            case 2:
                textToAdd = inCopyLesser.readAll();
                break;
            }
            textHolders.append(new QPlainTextEdit);
            textHolders.at(i)->appendPlainText(textToAdd);
            textHolders.at(i)->setReadOnly(true);
            QTextCursor cursor = textHolders.at(i)->textCursor();
            cursor.movePosition(QTextCursor::Start);
            textHolders.at(i)->setTextCursor(cursor);
        }
        QVBoxLayout *v1 = new QVBoxLayout, *v2 = new QVBoxLayout, *v3 = new QVBoxLayout;
        v1->addWidget(lb1);
        v1->addWidget(textHolders.at(0));
        v2->addWidget(lb2);
        v2->addWidget(textHolders.at(1));
        v3->addWidget(lb3);
        v3->addWidget(textHolders.at(2));
        QHBoxLayout *hbox = new QHBoxLayout;
        hbox->addLayout(v1);
        hbox->addLayout(v2);
        hbox->addLayout(v3);
        dialog->setLayout(hbox);
        dialog->setFixedSize(1100,600);
        dialog->setWindowTitle("Copyright");
        dialog->exec();
        delete dialog;
    }
    else QMessageBox::about(this,"Failed To Open","Failed To Open One Or More Copyright Files.");
}
