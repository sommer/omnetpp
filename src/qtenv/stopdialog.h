//==========================================================================
//  STOPDIALOG.H - part of
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

#ifndef STOPDIALOG_H
#define STOPDIALOG_H

#include <QDialog>

namespace Ui {
class StopDialog;
}

class StopDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StopDialog(QWidget *parent = 0);
    ~StopDialog();

public slots:
    void onClickStop();
    void onClickUpdate();
    void stopDialogAutoupdate();
protected:
    virtual void keyPressEvent(QKeyEvent *e);

private:
    Ui::StopDialog *ui;
};

#endif // STOPDIALOG_H
