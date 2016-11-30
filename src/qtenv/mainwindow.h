//==========================================================================
//  MAINWINDOW.H - part of
//
//                     OMNeT++/OMNEST
//            Discrete System Simulation in C++
//
//==========================================================================

/*--------------------------------------------------------------*
  Copyright (C) 1992-2015 Andras Varga
  Copyright (C) 2006-2015 OpenSim Ltd.

  This file is distributed WITHOUT ANY WARRANTY. See the file
  `license' for details on this and other legal matters.
*--------------------------------------------------------------*/

#ifndef __OMNETPP_QTENV_MAINWINDOW_H
#define __OMNETPP_QTENV_MAINWINDOW_H

#include <set>
#include "ui_mainwindow.h"
#include <QMainWindow>
#include <QModelIndex>
#include "qtenv.h"

class QGraphicsScene;
class QStandardItem;
class QTreeView;
class QGraphicsView;
class QWidget;
class QSlider;
class QSplitter;
class QLabel;

namespace omnetpp {

class cObject;
class cEvent;
class cCanvas;
class cMessage;

namespace qtenv {

class Qtenv;
class Inspector;
class StopDialog;
class AnimationControllerDialog;
class FileEditor;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    // stores actions and inspectors that have been disabled
    // only for layouting (so we can restore the state later)
    std::set<QObject*> disabledForLayouting;

public:
    explicit MainWindow(Qtenv *env, QWidget *parent = 0);
    ~MainWindow();

    void updateSimtimeDisplay();
    void updateStatusDisplay();
    void updateNetworkRunDisplay();

    QWidget *getMainInspectorArea() { return ui->mainArea; }
    QWidget *getObjectTreeArea() { return ui->treeView; }
    QWidget *getObjectInspectorArea() { return ui->objectInspector; }
    QWidget *getLogInspectorArea() { return ui->logInspector; }
    QWidget *getTimeLineArea() { return ui->timeLine; }
    QAction *getStopAction() { return ui->actionStop; }
    QAction *getFindObjectsAction() { return ui->actionFindInspectObjects; }

    void storeGeometry();
    void restoreGeometry();

    QSize sizeHint() const override { return QSize(1100, 700); }

    //menuproc.tcl
    bool isRunning();
    void setGuiForRunmode(RunMode runMode, bool untilMode = false);
    void setRunUntilModule(Inspector *insp = nullptr);
    bool networkReady();
    void runUntilMsg(cMessage *msg, RunMode runMode);
    void excludeMessageFromAnimation(cObject *msg);

    // the slider value is an integer, we divide it by 100 to get a double value, then raise it to 10's exponent
    static int playbackSpeedToSliderValue(double speed) { return std::round(std::log10(speed) * 100); }
    static double sliderValueToPlaybackSpeed(int value) { return std::pow(10, value / 100.0); }

public slots:
    void on_actionOneStep_triggered();
    void on_actionQuit_triggered();
    void on_actionRun_triggered() { runSimulation(RUNMODE_NORMAL); }
    void on_actionSetUpConfiguration_triggered();
    void on_actionFastRun_triggered() { runSimulation(RUNMODE_FAST); }
    void on_actionExpressRun_triggered() { runSimulation(RUNMODE_EXPRESS); }
    void on_actionRunUntil_triggered();
    void onSliderValueChanged(int value);
    void on_actionRebuildNetwork_triggered();
    void closeEvent(QCloseEvent *event);
    void initialSetUpConfiguration();
    void on_actionPreferences_triggered();
    void on_actionStatusDetails_triggered();
    void on_actionFindInspectObjects_triggered();
    void on_actionStop_triggered();
    void on_actionDebugNextEvent_triggered();
    void on_actionEventlogRecording_triggered();
    void on_actionSetUpUnconfiguredNetwork_triggered();
    void on_actionAbout_OMNeT_Qtenv_triggered();

    void showFindObjectsDialog(cObject *obj);

    // without emitting the change signal!
    void updateSpeedSlider();

    // disables all actions and inspectors except the stopAction and ModuleInspectors
    // and saves the disabled objects into disabledForLayouting.
    // the exit counterpart must be called at least once between calls
    void enterLayoutingMode();
    // restores the changes made by the function above.
    // will do no harm if called multiple times, or without any enterLayoutingMode call at all
    void exitLayoutingMode();

protected slots:
    void on_actionVerticalLayout_triggered(bool checked);
    void on_actionHorizontalLayout_triggered(bool checked);
    void on_actionFlipWindowLayout_triggered();
    void on_actionTimeline_toggled(bool isSunken);

private slots:
    void onSplitterMoved(int, int);
    void onSimTimeLabelContextMenuRequested(QPoint pos);
    void onSimTimeLabelGroupingTriggered();
    void onSimTimeLabelUnitsTriggered();
    void onEventNumLabelContextMenuRequested(QPoint pos);
    void onEventNumLabelGroupingTriggered();
    void on_actionLoadNEDFile_triggered();
    void on_actionOpenPrimaryIniFile_triggered();
    void on_actionCreate_Snapshot_triggered();
    void on_actionConcludeSimulation_triggered();
    void on_actionNetwork_triggered();
    void on_actionScheduledEvents_triggered();
    void on_actionSimulation_triggered();
    void on_actionNEDComponentTypes_triggered();
    void on_actionRegisteredClasses_triggered();
    void on_actionNED_Functions_triggered();
    void on_actionRegistered_Enums_triggered();
    void on_actionSupportedConfigurationOption_triggered();
    void on_actionInspectByPointer_triggered();
    void on_actionRecordVideo_toggled(bool checked);
    void on_actionShowAnimationParams_toggled(bool checked);

signals:
    void setNewNetwork();
    void closed();

private:
    Ui::MainWindow *ui;
    Qtenv *env;
    StopDialog *stopDialog = nullptr;
    QSlider *slider;
    QList<int> timeLineSize;
    bool showStatusDetails;
    static QString aboutText;
    QList<int> defaultTimeLineSize;
    FileEditor *fileEditor;
    QLabel *simTimeLabel, *eventNumLabel;
    bool simTimeUnits;

    enum DigitGrouping
    {
        SPACE,
        COMMA,
        APOSTROPHE,
        NONE
    } simTimeDigitGrouping, eventNumDigitGrouping;

    bool checkRunning();
    void runSimulation(RunMode runMode);

    void updatePerformanceDisplay();
    void updateNextEventDisplay();
    int getObjectId(cEvent *object);
    const char *getObjectShortTypeName(cObject *object);
    const char *stripNamespace(const char *className);

    bool networkPresent();
    bool isSimulationOk();

    void busy(QString msg = "");
    void copyToClipboard(cObject *object, int what);

    void saveSplitter(QString prefName, QSplitter *splitter);
    void restoreSplitter(QString prefName, QSplitter *splitter, const QList<int> &defaultSizes = QList<int>());

    void reflectRecordEventlog();

    void showStopDialog();
    void closeStopDialog();

    int inputBox(const QString &title, const QString &prompt, QString &variable);

    void updateSimTimeLabel();
    void updateEventNumLabel();

    void configureNetwork();

    bool exitOmnetpp();
};

} // namespace qtenv
} // namespace omnetpp

#endif // __OMNETPP_QTENV_MAINWINDOW_H
