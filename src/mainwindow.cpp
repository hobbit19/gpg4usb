/*
 *      mainwindow.cpp
 *
 *      Copyright 2008 gpg4usb-team <gpg4usb@cpunk.de>
 *
 *      This file is part of gpg4usb.
 *
 *      Gpg4usb is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      Gpg4usb is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with gpg4usb.  If not, see <http://www.gnu.org/licenses/>
 */

#include "mainwindow.h"

MainWindow::MainWindow()
{
    mCtx = new GpgME::GpgContext();

    /* get path were app was started */
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    edit = new TextEdit();
    setCentralWidget(edit);

    /* the list of Keys available*/
    mKeyList = new KeyList(mCtx);

    /* List of binary Attachments */
    attachmentDockCreated = false;

    /* Variable containing if restart is needed */
    this->slotSetRestartNeeded(false);

    keyMgmt = new KeyMgmt(mCtx, this);
    keyMgmt->hide();
    /* test attachmentdir for files alll 15s */
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(slotCheckAttachmentFolder()));
    timer->start(5000);

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    createDockWindows();

    connect(edit->tabWidget,SIGNAL(currentChanged(int)),this,SLOT(slotDisableTabActions(int)));

    mKeyList->addMenuAction(appendSelectedKeysAct);
    mKeyList->addMenuAction(copyMailAddressToClipboardAct);
    mKeyList->addMenuAction(showKeyDetailsAct);
    mKeyList->addMenuAction(refreshKeysFromKeyserverAct);
    mKeyList->addMenuAction(uploadKeyToServerAct);

    restoreSettings();

    // open filename if provided as first command line parameter
    QStringList args = qApp->arguments();
    if (args.size() > 1) {
        if (!args[1].startsWith("-")) {
            if (QFile::exists(args[1]))
                edit->loadFile(args[1]);
        }
    }
    edit->curTextPage()->setFocus();
    this->setWindowTitle(qApp->applicationName());
    this->show();

    // Show wizard, if the don't show wizard message box wasn't checked
    // and keylist doesn't contain a private key
    QSettings settings;
    if (settings.value("wizard/showWizard",true).toBool() || !settings.value("wizard/nextPage").isNull()) {
        slotStartWizard();
    }
}

void MainWindow::restoreSettings()
{
    // state sets pos & size of dock-widgets
    this->restoreState(settings.value("window/windowState").toByteArray());

    // Restore window size & location
    if (settings.value("window/windowSave").toBool()) {
        QPoint pos = settings.value("window/pos", QPoint(100, 100)).toPoint();
        QSize size = settings.value("window/size", QSize(800, 450)).toSize();
        this->resize(size);
        this->move(pos);
    } else {
        this->resize(QSize(800, 450));
        this->move(QPoint(100, 100));
    }

    // Iconsize
    QSize iconSize = settings.value("toolbar/iconsize", QSize(24, 24)).toSize();
    this->setIconSize(iconSize);

    importButton->setIconSize(iconSize);
    fileEncButton->setIconSize(iconSize);
    // set list of keyserver if not defined
    QStringList *keyServerDefaultList;
    keyServerDefaultList = new QStringList("http://pgp.mit.edu");
    keyServerDefaultList->append("http://pool.sks-keyservers.net");
    keyServerDefaultList->append("http://subkeys.pgp.net");

    QStringList keyServerList = settings.value("keyserver/keyServerList", *keyServerDefaultList).toStringList();
    settings.setValue("keyserver/keyServerList", keyServerList);

    // set default keyserver, if it's not set
    QString defaultKeyServer = settings.value("keyserver/defaultKeyServer", QString("http://pgp.mit.edu")).toString();
    settings.setValue("keyserver/defaultKeyServer", defaultKeyServer);

    // Iconstyle
    Qt::ToolButtonStyle buttonStyle = static_cast<Qt::ToolButtonStyle>(settings.value("toolbar/iconstyle", Qt::ToolButtonTextUnderIcon).toUInt());
    this->setToolButtonStyle(buttonStyle);
    importButton->setToolButtonStyle(buttonStyle);
    fileEncButton->setToolButtonStyle(buttonStyle);

    // Checked Keys
    if (settings.value("keys/keySave").toBool()) {
        QStringList keyIds = settings.value("keys/keyList").toStringList();
        mKeyList->setChecked(&keyIds);
    }
}

void MainWindow::saveSettings()
{
    // window position and size
    settings.setValue("window/windowState", saveState());
    settings.setValue("window/pos", pos());
    settings.setValue("window/size", size());

    // keyid-list of private checked keys
    if (settings.value("keys/keySave").toBool()) {
        QStringList *keyIds = mKeyList->getPrivateChecked();
        if (!keyIds->isEmpty()) {
            settings.setValue("keys/keyList", *keyIds);
        } else {
            settings.setValue("keys/keyList", "");
        }
    } else  {
        settings.remove("keys/keyList");
    }
}

void MainWindow::createActions()
{
    /* Main Menu
      */
    newTabAct = new QAction(tr("&New"), this);
    newTabAct->setIcon(QIcon(":misc_doc.png"));
    QList<QKeySequence> newTabActShortcutList;
    newTabActShortcutList.append(QKeySequence (Qt::CTRL + Qt::Key_N));
    newTabActShortcutList.append(QKeySequence (Qt::CTRL + Qt::Key_T));
    newTabAct->setShortcuts(newTabActShortcutList);
    newTabAct->setToolTip(tr("Open a new file"));
    connect(newTabAct, SIGNAL(triggered()), edit, SLOT(slotNewTab()));

    openAct = new QAction(tr("&Open..."), this);
    openAct->setIcon(QIcon(":fileopen.png"));
    openAct->setShortcut(QKeySequence::Open);
    openAct->setToolTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), edit, SLOT(slotOpen()));

    saveAct = new QAction(tr("&Save"), this);
    saveAct->setIcon(QIcon(":filesave.png"));
    saveAct->setShortcut(QKeySequence::Save);
    saveAct->setToolTip(tr("Save the current File"));
    connect(saveAct, SIGNAL(triggered()), edit, SLOT(slotSave()));

    saveAsAct = new QAction(tr("Save &As")+"...", this);
    saveAsAct->setIcon(QIcon(":filesaveas.png"));
    saveAsAct->setShortcut(QKeySequence::SaveAs);
    saveAsAct->setToolTip(tr("Save the current File as..."));
    connect(saveAsAct, SIGNAL(triggered()), edit, SLOT(slotSaveAs()));

    printAct = new QAction(tr("&Print"), this);
    printAct->setIcon(QIcon(":fileprint.png"));
    printAct->setShortcut(QKeySequence::Print);
    printAct->setToolTip(tr("Print Document"));
    connect(printAct, SIGNAL(triggered()), edit, SLOT(slotPrint()));

    closeTabAct = new QAction(tr("&Close"), this);
    closeTabAct->setShortcut(QKeySequence::Close);
    closeTabAct->setToolTip(tr("Close file"));
    connect(closeTabAct, SIGNAL(triggered()), edit, SLOT(slotCloseTab()));

    quitAct = new QAction(tr("&Quit"), this);
    quitAct->setShortcut(QKeySequence::Quit);
    quitAct->setIcon(QIcon(":exit.png"));
    quitAct->setToolTip(tr("Quit Program"));
    connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));

    /* Edit Menu
     */
    undoAct = new QAction(tr("&Undo"), this);
    undoAct->setShortcut(QKeySequence::Undo);
    undoAct->setToolTip(tr("Undo Last Edit Action"));
    connect(undoAct, SIGNAL(triggered()), edit, SLOT(slotUndo()));

    redoAct = new QAction(tr("&Redo"), this);
    redoAct->setShortcut(QKeySequence::Redo);
    redoAct->setToolTip(tr("Redo Last Edit Action"));
    connect(redoAct, SIGNAL(triggered()), edit, SLOT(slotRedo()));

    zoomInAct = new QAction(tr("Zoom In"), this);
    zoomInAct->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAct, SIGNAL(triggered()), edit, SLOT(slotZoomIn()));

    zoomOutAct = new QAction(tr("Zoom Out"), this);
    zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAct, SIGNAL(triggered()), edit, SLOT(slotZoomOut()));

    pasteAct = new QAction(tr("&Paste"), this);
    pasteAct->setIcon(QIcon(":button_paste.png"));
    pasteAct->setShortcut(QKeySequence::Paste);
    pasteAct->setToolTip(tr("Paste Text From Clipboard"));
    connect(pasteAct, SIGNAL(triggered()), edit, SLOT(slotPaste()));

    cutAct = new QAction(tr("Cu&t"), this);
    cutAct->setIcon(QIcon(":button_cut.png"));
    cutAct->setShortcut(QKeySequence::Cut);
    cutAct->setToolTip(tr("Cut the current selection's contents to the "
                          "clipboard"));
    connect(cutAct, SIGNAL(triggered()), edit, SLOT(slotCut()));

    copyAct = new QAction(tr("&Copy"), this);
    copyAct->setIcon(QIcon(":button_copy.png"));
    copyAct->setShortcut(QKeySequence::Copy);
    copyAct->setToolTip(tr("Copy the current selection's contents to the "
                           "clipboard"));
    connect(copyAct, SIGNAL(triggered()), edit, SLOT(slotCopy()));

    quoteAct = new QAction(tr("&Quote"), this);
    quoteAct->setIcon(QIcon(":quote.png"));
    quoteAct->setToolTip(tr("Quote whole text"));
    connect(quoteAct, SIGNAL(triggered()), edit, SLOT(slotQuote()));

    selectallAct = new QAction(tr("Select &All"), this);
    selectallAct->setIcon(QIcon(":edit.png"));
    selectallAct->setShortcut(QKeySequence::SelectAll);
    selectallAct->setToolTip(tr("Select the whole text"));
    connect(selectallAct, SIGNAL(triggered()), edit, SLOT(slotSelectAll()));

    findAct = new QAction(tr("&Find"), this);
    findAct->setShortcut(QKeySequence::Find);
    findAct->setToolTip(tr("Find a word"));
    connect(findAct, SIGNAL(triggered()), this, SLOT(slotFind()));

    cleanDoubleLinebreaksAct = new QAction(tr("Remove &spacing"), this);
    cleanDoubleLinebreaksAct->setIcon(QIcon(":format-line-spacing-triple.png"));
    //cleanDoubleLineBreaksAct->setShortcut(QKeySequence::SelectAll);
    cleanDoubleLinebreaksAct->setToolTip(tr("Remove double linebreaks, e.g. in pasted text from webmailer"));
    connect(cleanDoubleLinebreaksAct, SIGNAL(triggered()), this, SLOT(slotCleanDoubleLinebreaks()));

    openSettingsAct = new QAction(tr("Se&ttings"), this);
    openSettingsAct->setToolTip(tr("Open settings dialog"));
    openSettingsAct->setShortcut(QKeySequence::Preferences);
    connect(openSettingsAct, SIGNAL(triggered()), this, SLOT(slotOpenSettingsDialog()));

    /* Crypt Menu
     */
    encryptAct = new QAction(tr("&Encrypt"), this);
    encryptAct->setIcon(QIcon(":encrypted.png"));
    encryptAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
    encryptAct->setToolTip(tr("Encrypt Message"));
    connect(encryptAct, SIGNAL(triggered()), this, SLOT(slotEncrypt()));

    decryptAct = new QAction(tr("&Decrypt"), this);
    decryptAct->setIcon(QIcon(":decrypted.png"));
    decryptAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
    decryptAct->setToolTip(tr("Decrypt Message"));
    connect(decryptAct, SIGNAL(triggered()), this, SLOT(slotDecrypt()));

    /*
     * File encryption submenu
     */
    fileEncryptAct = new QAction(tr("&Encrypt File"), this);
    fileEncryptAct->setToolTip(tr("Encrypt File"));
    connect(fileEncryptAct, SIGNAL(triggered()), this, SLOT(slotFileEncrypt()));

    fileDecryptAct = new QAction(tr("&Decrypt File"), this);
    fileDecryptAct->setToolTip(tr("Decrypt File"));
    connect(fileDecryptAct, SIGNAL(triggered()), this, SLOT(slotFileDecrypt()));

    fileSignAct = new QAction(tr("&Sign File"), this);
    fileSignAct->setToolTip(tr("Sign File"));
    connect(fileSignAct, SIGNAL(triggered()), this, SLOT(slotFileSign()));

    fileVerifyAct = new QAction(tr("&Verify File"), this);
    fileVerifyAct->setToolTip(tr("Verify File"));
    connect(fileVerifyAct, SIGNAL(triggered()), this, SLOT(slotFileVerify()));


    signAct = new QAction(tr("&Sign"), this);
    signAct->setIcon(QIcon(":signature.png"));
    signAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_I));
    signAct->setToolTip(tr("Sign Message"));
    connect(signAct, SIGNAL(triggered()), this, SLOT(slotSign()));

    verifyAct = new QAction(tr("&Verify"), this);
    verifyAct->setIcon(QIcon(":verify.png"));
    verifyAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_V));
    verifyAct->setToolTip(tr("Verify Message"));
    connect(verifyAct, SIGNAL(triggered()), this, SLOT(slotVerify()));

    /* Key Menu
     */

    importKeyFromEditAct = new QAction(tr("&Editor"), this);
    importKeyFromEditAct->setIcon(QIcon(":txt.png"));
    importKeyFromEditAct->setToolTip(tr("Import New Key From Editor"));
    connect(importKeyFromEditAct, SIGNAL(triggered()), this, SLOT(slotImportKeyFromEdit()));

    openKeyManagementAct = new QAction(tr("Manage &keys"), this);
    openKeyManagementAct->setIcon(QIcon(":keymgmt.png"));
    openKeyManagementAct->setToolTip(tr("Open Keymanagement"));
    connect(openKeyManagementAct, SIGNAL(triggered()), this, SLOT(slotOpenKeyManagement()));

    /* About Menu
     */
    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setIcon(QIcon(":help.png"));
    aboutAct->setToolTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(slotAbout()));

    openHelpAct = new QAction(tr("Integrated Help"), this);
    openHelpAct->setToolTip(tr("Open integrated Help"));
    connect(openHelpAct, SIGNAL(triggered()), this, SLOT(slotOpenHelp()));

    openTutorialAct = new QAction(tr("Online &Tutorials"), this);
    openTutorialAct->setToolTip(tr("Open Online Tutorials"));
    connect(openTutorialAct, SIGNAL(triggered()), this, SLOT(slotOpenTutorial()));

    openTranslateAct = new QAction(tr("Translate gpg4usb"), this);
    openTranslateAct->setToolTip(tr("Translate gpg4usb yourself"));
    connect(openTranslateAct, SIGNAL(triggered()), this, SLOT(slotOpenTranslate()));

    startWizardAct= new QAction(tr("Open &Wizard"), this);
    startWizardAct->setToolTip(tr("Open the wizard"));
    connect(startWizardAct, SIGNAL(triggered()), this, SLOT(slotStartWizard()));

    /* Popup-Menu-Action for KeyList
     */
    appendSelectedKeysAct = new QAction(tr("Append Selected Key(s) To Text"), this);
    appendSelectedKeysAct->setToolTip(tr("Append The Selected Keys To Text in Editor"));
    connect(appendSelectedKeysAct, SIGNAL(triggered()), this, SLOT(slotAppendSelectedKeys()));

    copyMailAddressToClipboardAct = new QAction(tr("Copy EMail-address"), this);
    copyMailAddressToClipboardAct->setToolTip(tr("Copy selected EMailaddress to clipboard"));
    connect(copyMailAddressToClipboardAct, SIGNAL(triggered()), this, SLOT(slotCopyMailAddressToClipboard()));

    // TODO: find central place for shared actions, to avoid code-duplication with keymgmt.cpp
    showKeyDetailsAct = new QAction(tr("Show Keydetails"), this);
    showKeyDetailsAct->setToolTip(tr("Show Details for this Key"));
    connect(showKeyDetailsAct, SIGNAL(triggered()), this, SLOT(slotShowKeyDetails()));

    refreshKeysFromKeyserverAct = new QAction(tr("Refresh key from keyserver"), this);
    refreshKeysFromKeyserverAct->setToolTip(tr("Refresh key from default keyserver"));
    connect(refreshKeysFromKeyserverAct, SIGNAL(triggered()), this, SLOT(refreshKeysFromKeyserver()));

    uploadKeyToServerAct = new QAction(tr("Upload Key(s) To Server"), this);
    uploadKeyToServerAct->setToolTip(tr("Upload The Selected Keys To Server"));
    connect(uploadKeyToServerAct, SIGNAL(triggered()), this, SLOT(uploadKeyToServer()));
     /* Key-Shortcuts for Tab-Switchung-Action
     */
    switchTabUpAct = new QAction(this);
    switchTabUpAct->setShortcut(QKeySequence::NextChild);
    connect(switchTabUpAct, SIGNAL(triggered()), edit, SLOT(slotSwitchTabUp()));
    this->addAction(switchTabUpAct);

    switchTabDownAct = new QAction(this);
    switchTabDownAct->setShortcut(QKeySequence::PreviousChild);
    connect(switchTabDownAct, SIGNAL(triggered()), edit, SLOT(slotSwitchTabDown()));
    this->addAction(switchTabDownAct);

    cutPgpHeaderAct = new QAction(tr("Remove PGP Header"), this);
    connect(cutPgpHeaderAct, SIGNAL(triggered()), this, SLOT(slotCutPgpHeader()));

    addPgpHeaderAct = new QAction(tr("Add PGP Header"), this);
    connect(addPgpHeaderAct, SIGNAL(triggered()), this, SLOT(slotAddPgpHeader()));
}

void MainWindow::slotDisableTabActions(int number)
{
    bool disable;

    if (number == -1 ) {
        disable = true;
    } else {
        disable= false;
    }
    printAct->setDisabled(disable);
    saveAct->setDisabled(disable);
    saveAsAct->setDisabled(disable);
    quoteAct->setDisabled(disable);
    cutAct->setDisabled(disable);
    copyAct->setDisabled(disable);
    pasteAct->setDisabled(disable);
    closeTabAct->setDisabled(disable);
    selectallAct->setDisabled(disable);
    findAct->setDisabled(disable);
    verifyAct->setDisabled(disable);
    signAct->setDisabled(disable);
    encryptAct->setDisabled(disable);
    decryptAct->setDisabled(disable);

    redoAct->setDisabled(disable);
    undoAct->setDisabled(disable);
    zoomOutAct->setDisabled(disable);
    zoomInAct->setDisabled(disable);
    cleanDoubleLinebreaksAct->setDisabled(disable);
    quoteAct->setDisabled(disable);
    appendSelectedKeysAct->setDisabled(disable);
    importKeyFromEditAct->setDisabled(disable);

    cutPgpHeaderAct->setDisabled(disable);
    addPgpHeaderAct->setDisabled(disable);
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newTabAct);
    fileMenu->addAction(openAct);
    fileMenu->addSeparator();
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(printAct);
    fileMenu->addSeparator();
    fileMenu->addAction(closeTabAct);
    fileMenu->addAction(quitAct);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(undoAct);
    editMenu->addAction(redoAct);
    editMenu->addSeparator();
    editMenu->addAction(zoomInAct);
    editMenu->addAction(zoomOutAct);
    editMenu->addSeparator();
    editMenu->addAction(copyAct);
    editMenu->addAction(cutAct);
    editMenu->addAction(pasteAct);
    editMenu->addAction(selectallAct);
    editMenu->addAction(findAct);
    editMenu->addSeparator();
    editMenu->addAction(quoteAct);
    editMenu->addAction(cleanDoubleLinebreaksAct);
    editMenu->addSeparator();
    editMenu->addAction(openSettingsAct);

    fileEncMenu = new QMenu(tr("&File..."));
    fileEncMenu->addAction(fileEncryptAct);
    fileEncMenu->addAction(fileDecryptAct);
    fileEncMenu->addAction(fileSignAct);
    fileEncMenu->addAction(fileVerifyAct);

    cryptMenu = menuBar()->addMenu(tr("&Crypt"));
    cryptMenu->addAction(encryptAct);
    cryptMenu->addAction(decryptAct);
    cryptMenu->addSeparator();
    cryptMenu->addAction(signAct);
    cryptMenu->addAction(verifyAct);
    cryptMenu->addSeparator();
    cryptMenu->addMenu(fileEncMenu);

    keyMenu = menuBar()->addMenu(tr("&Keys"));
    importKeyMenu = keyMenu->addMenu(tr("&Import Key From..."));
    importKeyMenu->setIcon(QIcon(":key_import.png"));
    importKeyMenu->addAction(keyMgmt->importKeyFromFileAct);
    importKeyMenu->addAction(importKeyFromEditAct);
    importKeyMenu->addAction(keyMgmt->importKeyFromClipboardAct);
    importKeyMenu->addAction(keyMgmt->importKeyFromKeyServerAct);
    importKeyMenu->addAction(keyMgmt->importKeyFromKeyServerAct);
    keyMenu->addAction(openKeyManagementAct);

    steganoMenu = menuBar()->addMenu(tr("&Steganography"));
    steganoMenu->addAction(cutPgpHeaderAct);
    steganoMenu->addAction(addPgpHeaderAct);

    // Hide menu, when steganography menu is disabled in settings
    if(!settings.value("advanced/steganography").toBool()) {
        this->menuBar()->removeAction(steganoMenu->menuAction());
    }

    viewMenu = menuBar()->addMenu(tr("&View"));

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(openHelpAct);
    helpMenu->addAction(startWizardAct);
    helpMenu->addSeparator();
    helpMenu->addAction(openTutorialAct);
    helpMenu->addAction(openTranslateAct);
    helpMenu->addSeparator();
    helpMenu->addAction(aboutAct);

}

void MainWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    fileToolBar->setObjectName("fileToolBar");
    fileToolBar->addAction(newTabAct);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(saveAct);
    fileToolBar->hide();
    viewMenu->addAction(fileToolBar->toggleViewAction());

    cryptToolBar = addToolBar(tr("Crypt"));
    cryptToolBar->setObjectName("cryptToolBar");
    cryptToolBar->addAction(encryptAct);
    cryptToolBar->addAction(decryptAct);
    cryptToolBar->addAction(signAct);
    cryptToolBar->addAction(verifyAct);
    //cryptToolBar->addAction(fileEncryptionAct);
    viewMenu->addAction(cryptToolBar->toggleViewAction());

    keyToolBar = addToolBar(tr("Key"));
    keyToolBar->setObjectName("keyToolBar");
    keyToolBar->addAction(openKeyManagementAct);
    viewMenu->addAction(keyToolBar->toggleViewAction());

    editToolBar = addToolBar(tr("Edit"));
    editToolBar->setObjectName("editToolBar");
    editToolBar->addAction(copyAct);
    editToolBar->addAction(pasteAct);
    editToolBar->addAction(selectallAct);
    viewMenu->addAction(editToolBar->toggleViewAction());

    specialEditToolBar = addToolBar(tr("Special edit"));
    specialEditToolBar->setObjectName("specialEditToolBar");
    specialEditToolBar->addAction(quoteAct);
    specialEditToolBar->addAction(cleanDoubleLinebreaksAct);
    viewMenu->addAction(specialEditToolBar->toggleViewAction());

    // Add dropdown menu for key import to keytoolbar
    importButton = new QToolButton();
    importButton->setMenu(importKeyMenu);
    importButton->setPopupMode(QToolButton::InstantPopup);
    importButton->setIcon(QIcon(":key_import.png"));
    importButton->setToolTip(tr("Import key from..."));
    importButton->setText(tr("Import key"));
    keyToolBar->addWidget(importButton);

    // Add dropdown menu for file encryption/decryption to crypttoolbar
    fileEncButton = new QToolButton();
    fileEncButton->setMenu(fileEncMenu);
    fileEncButton->setPopupMode(QToolButton::InstantPopup);
    fileEncButton->setIcon(QIcon(":fileencryption.png"));
    fileEncButton->setToolTip(tr("Encrypt or decrypt File"));
    fileEncButton->setText(tr("File.."));

    cryptToolBar->addWidget(fileEncButton);

}

void MainWindow::createStatusBar()
{
    QWidget *statusBarBox = new QWidget();
    QHBoxLayout *statusBarBoxLayout = new QHBoxLayout();
    QPixmap *pixmap;

    // icon which should be shown if there are files in attachments-folder
    pixmap = new QPixmap(":statusbar_icon.png");
    statusBarIcon = new QLabel(statusBar());
    statusBarIcon->setPixmap(*pixmap);
    statusBar()->insertPermanentWidget(0,statusBarIcon,0);
    statusBarIcon->hide();
    statusBar()->showMessage(tr("Ready"),2000);
    statusBarBox->setLayout(statusBarBoxLayout);
}

void MainWindow::createDockWindows()
{
    /* KeyList-Dockwindow
     */
    keylistDock = new QDockWidget(tr("Encrypt for:"), this);
    keylistDock->setObjectName("EncryptDock");
    keylistDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, keylistDock);
    keylistDock->setWidget(mKeyList);
    viewMenu->addAction(keylistDock->toggleViewAction());

    /* Attachments-Dockwindow
      */
    if(settings.value("mime/parseMime").toBool()) {
        createAttachmentDock();
    }
}

void MainWindow::createAttachmentDock() {
    if (attachmentDockCreated) {
        return;
    }
    mAttachments = new Attachments();
    attachmentDock = new QDockWidget(tr("Attached files:"), this);
    attachmentDock->setObjectName("AttachmentDock");
    attachmentDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, attachmentDock);
    attachmentDock->setWidget(mAttachments);
    // hide till attachment is decrypted
    viewMenu->addAction(attachmentDock->toggleViewAction());
    attachmentDock->hide();
    attachmentDockCreated = true;
}

void MainWindow::closeAttachmentDock() {
    if (!attachmentDockCreated) {
        return;
    }
    attachmentDock->close();
    attachmentDock->deleteLater();
    attachmentDockCreated = false;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    /*
     * ask to save changes, if there are
     * modified documents in any tab
     */
    if (edit->maybeSaveAnyTab()) {
        saveSettings();
        event->accept();
    } else {
        event->ignore();
    }

    // clear password from memory
    mCtx->clearPasswordCache();
}

void MainWindow::slotAbout()
{
    new AboutDialog(this);
}

void MainWindow::slotOpenTranslate()
{
    QDesktopServices::openUrl(QUrl("http://gpg4usb.cpunk.de/docu_translate.html"));
}

void MainWindow::slotOpenTutorial()
{
    QDesktopServices::openUrl(QUrl("http://gpg4usb.cpunk.de/docu.html"));
}

void MainWindow::slotOpenHelp() {
    slotOpenHelp("docu.html");
}

void MainWindow::slotOpenHelp(const QString page)
{
    edit->slotNewHelpTab("help", "file:" + qApp->applicationDirPath() + "/help/" + page);
}

void MainWindow::slotSetStatusBarText(QString text)
{
    statusBar()->showMessage(text,20000);
}

void MainWindow::slotStartWizard()
{
    Wizard *wizard = new Wizard(mCtx,keyMgmt,this);
    wizard->show();
    wizard->setModal(true);
}

/*
  * if this is mime, split text and attachments...
  * message contains only text afterwards
  */
void MainWindow::parseMime(QByteArray *message)
{
    /*if (! Mime::isMultipart(message)) {
        qDebug() << "no multipart";
        return;
    }*/
    //qDebug() << "multipart";

    QString pText;
    bool showmadock = false;

    Mime *mime = new Mime(message);
    foreach(MimePart tmp, mime->parts()) {
        if (tmp.header.getValue("Content-Type") == "text/plain"
                && tmp.header.getValue("Content-Transfer-Encoding") != "base64") {

            QByteArray body;
            if (tmp.header.getValue("Content-Transfer-Encoding") == "quoted-printable") {
                Mime::quotedPrintableDecode(tmp.body, body);
            } else {
                body = tmp.body;
            }
            pText.append(QString(body));
        } else {
            (mAttachments->addMimePart(&tmp));
            showmadock = true;
        }
    }
    *message = pText.toUtf8();
    if (showmadock) {
        attachmentDock->show();
    }
}

void MainWindow::slotCheckAttachmentFolder() {
    // TODO: always check?
    if(!settings.value("mime/parseMime").toBool()) {
        return;
    }

    QString attachmentDir = qApp->applicationDirPath() + "/attachments/";
    // filenum minus . and ..
    int filenum = QDir(attachmentDir).count() - 2 ;
    if(filenum > 0) {
        QString statusText;
        if(filenum == 1) {
            statusText = tr("There is one unencrypted file in attachment folder");
        } else {
            statusText = tr("There are ") + QString::number(filenum) +  tr(" unencrypted files in attachment folder");
        }
        statusBarIcon->setStatusTip(statusText);
        statusBarIcon->show();
    } else {
        statusBarIcon->hide();
    }
}

void MainWindow::slotImportKeyFromEdit()
{
    if (edit->tabCount()==0 || edit->slotCurPage() == 0) {
        return;
    }

    keyMgmt->slotImportKeys(edit->curTextPage()->toPlainText().toAscii());
}

void MainWindow::slotOpenKeyManagement()
{
    keyMgmt->show();
    keyMgmt->raise();
    keyMgmt->activateWindow();
}

void MainWindow::slotEncrypt()
{
    if (edit->tabCount()==0 || edit->slotCurPage() == 0) {
        return;
    }

    QStringList *uidList = mKeyList->getChecked();

    QByteArray *tmp = new QByteArray();
    if (mCtx->encrypt(uidList, edit->curTextPage()->toPlainText().toUtf8(), tmp)) {
        QString *tmp2 = new QString(*tmp);
        edit->slotFillTextEditWithText(*tmp2);
    }
}

void MainWindow::slotSign()
{
    if (edit->tabCount()==0 || edit->slotCurPage() == 0) {
        return;
    }

    QStringList *uidList = mKeyList->getPrivateChecked();

    QByteArray *tmp = new QByteArray();

    if (mCtx->sign(uidList, edit->curTextPage()->toPlainText().toUtf8(), tmp)) {
        edit->slotFillTextEditWithText(QString::fromUtf8(*tmp));
    }
}

void MainWindow::slotDecrypt()
{
    if (edit->tabCount()== 0 || edit->slotCurPage() == 0) {
        return;
    }

    QByteArray *decrypted = new QByteArray();
    QByteArray text = edit->curTextPage()->toPlainText().toAscii(); // TODO: toUtf8() here?
    mCtx->preventNoDataErr(&text);

    // try decrypt, if fail do nothing, especially don't replace text
    if(!mCtx->decrypt(text, decrypted)) {
        return;
    }

    /*
         *   1) is it mime (content-type:)
         *   2) parse header
         *   2) choose action depending on content-type
         */
    if(Mime::isMime(decrypted)) {
        Header header = Mime::getHeader(decrypted);
        // is it multipart, is multipart-parsing enabled
        if(header.getValue("Content-Type") == "multipart/mixed"
           && settings.value("mime/parseMime").toBool()) {
            parseMime(decrypted);
        } else if(header.getValue("Content-Type") == "text/plain"
                  && settings.value("mime/parseQP").toBool()){
            if (header.getValue("Content-Transfer-Encoding") == "quoted-printable") {
                QByteArray *decoded = new QByteArray();
                Mime::quotedPrintableDecode(*decrypted, *decoded);
                //TODO: remove header
                decrypted = decoded;
            }
        }
    }
    edit->slotFillTextEditWithText(QString::fromUtf8(*decrypted));
}

void MainWindow::slotFind()
{
    if (edit->tabCount()==0 || edit->curTextPage() == 0) {
        return;
    }

    // At first close verifynotification, if existing
    edit->slotCurPage()->closeNoteByClass("findwidget");

    FindWidget *fw = new FindWidget(this,edit->curTextPage());
    edit->slotCurPage()->showNotificationWidget(fw, "findWidget");

}

void MainWindow::slotVerify()
{
    if (edit->tabCount()==0 || edit->slotCurPage() == 0) {
        return;
    }

    // At first close verifynotification, if existing
    edit->slotCurPage()->closeNoteByClass("verifyNotification");

    // create new verfiy notification
    VerifyNotification *vn = new VerifyNotification(this, mCtx, mKeyList, edit->curTextPage());

    // if signing information is found, show the notification, otherwise close it
    if (vn->slotRefresh()) {
        edit->slotCurPage()->showNotificationWidget(vn, "verifyNotification");
    } else {
        vn->close();
    }
}

/*
 * Append the selected (not checked!) Key(s) To Textedit
 */
void MainWindow::slotAppendSelectedKeys()
{
    if (edit->tabCount()==0 || edit->slotCurPage() == 0) {
        return;
    }

    QByteArray *keyArray = new QByteArray();
    mCtx->exportKeys(mKeyList->getSelected(), keyArray);
    edit->curTextPage()->append(*keyArray);
}

void MainWindow::slotCopyMailAddressToClipboard()
{
    if (mKeyList->getSelected()->isEmpty()) {
        return;
    }

    gpgme_key_t key = mCtx->getKeyDetails(mKeyList->getSelected()->first());
    QClipboard *cb = QApplication::clipboard();
    QString mail = key->uids->email;
    cb->setText(mail);
}

void MainWindow::slotShowKeyDetails()
{
    if (mKeyList->getSelected()->isEmpty()) {
        return;
    }

    gpgme_key_t key = mCtx->getKeyDetails(mKeyList->getSelected()->first());
    if (key) {
        new KeyDetailsDialog(mCtx, key, this);
    }
}
void MainWindow::refreshKeysFromKeyserver()
{
    if (mKeyList->getSelected()->isEmpty()) {
        return;
    }

    KeyServerImportDialog *ksid = new KeyServerImportDialog(mCtx,mKeyList,this);
    ksid->slotImport(*mKeyList->getSelected());

}

void MainWindow::uploadKeyToServer()
{
    QByteArray *keyArray = new QByteArray();
    mCtx->exportKeys(mKeyList->getSelected(), keyArray);

    mKeyList->uploadKeyToServer(keyArray);
}

void MainWindow::slotFileEncrypt()
{
        QStringList *keyList;
        keyList = mKeyList->getChecked();
        new FileEncryptionDialog(mCtx, *keyList, FileEncryptionDialog::Encrypt, this);
}

void MainWindow::slotFileDecrypt()
{
        QStringList *keyList;
        keyList = mKeyList->getChecked();
        new FileEncryptionDialog(mCtx, *keyList, FileEncryptionDialog::Decrypt, this);
}

void MainWindow::slotFileSign()
{
        QStringList *keyList;
        keyList = mKeyList->getChecked();
        new FileEncryptionDialog(mCtx, *keyList, FileEncryptionDialog::Sign, this);
}

void MainWindow::slotFileVerify()
{
        QStringList *keyList;
        keyList = mKeyList->getChecked();
        new FileEncryptionDialog(mCtx, *keyList, FileEncryptionDialog::Verify, this);
}

void MainWindow::slotOpenSettingsDialog()
{

    QString preLang = settings.value("int/lang").toString();
    QString preKeydbPath = settings.value("gpgpaths/keydbpath").toString();

    new SettingsDialog(mCtx, this);
    // Iconsize
    QSize iconSize = settings.value("toolbar/iconsize", QSize(32, 32)).toSize();
    this->setIconSize(iconSize);
    importButton->setIconSize(iconSize);
    fileEncButton->setIconSize(iconSize);

    // Iconstyle
    Qt::ToolButtonStyle buttonStyle = static_cast<Qt::ToolButtonStyle>(settings.value("toolbar/iconstyle", Qt::ToolButtonTextUnderIcon).toUInt());
    this->setToolButtonStyle(buttonStyle);
    importButton->setToolButtonStyle(buttonStyle);
    fileEncButton->setToolButtonStyle(buttonStyle);

    // Mime-settings
    if(settings.value("mime/parseMime").toBool()) {
        createAttachmentDock();
    } else if(attachmentDockCreated) {
        closeAttachmentDock();
    }

    // restart mainwindow if necessary
    if(getRestartNeeded()) {
        if(edit->maybeSaveAnyTab()) {
            saveSettings();
            qApp->exit(RESTART_CODE);
        }
    }

    // steganography hide/show
    if(!settings.value("advanced/steganography").toBool()) {
        this->menuBar()->removeAction(steganoMenu->menuAction());
    } else {
        this->menuBar()->insertAction(viewMenu->menuAction(), steganoMenu->menuAction());
    }

}

void MainWindow::slotCleanDoubleLinebreaks()
{
    if (edit->tabCount()==0 || edit->slotCurPage() == 0) {
        return;
    }

    QString content = edit->curTextPage()->toPlainText();
    content.replace("\n\n", "\n");
    edit->slotFillTextEditWithText(content);
}

void MainWindow::slotAddPgpHeader() {
    if (edit->tabCount()==0 || edit->slotCurPage() == 0) {
        return;
    }

    QString content = edit->curTextPage()->toPlainText().trimmed();

    content.prepend("\n\n").prepend(GpgConstants::PGP_CRYPT_BEGIN);
    content.append("\n").append(GpgConstants::PGP_CRYPT_END);

    edit->slotFillTextEditWithText(content);
}

void MainWindow::slotCutPgpHeader() {

    if (edit->tabCount()==0 || edit->slotCurPage() == 0) {
        return;
    }

    QString content = edit->curTextPage()->toPlainText();
    int start = content.indexOf(GpgConstants::PGP_CRYPT_BEGIN);
    int end = content.indexOf(GpgConstants::PGP_CRYPT_END);

    if(start < 0 || end < 0) {
        return;
    }

    // remove head
    int headEnd = content.indexOf("\n\n", start) + 2 ;
    content.remove(start, headEnd-start);

    // remove tail
    end = content.indexOf(GpgConstants::PGP_CRYPT_END);
    content.remove(end, QString(GpgConstants::PGP_CRYPT_END).size());

    edit->slotFillTextEditWithText(content.trimmed());
}

void MainWindow::slotSetRestartNeeded(bool needed)
{
    this->restartNeeded = needed;
}

bool MainWindow::getRestartNeeded()
{
    return this->restartNeeded;
}
