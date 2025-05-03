/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *action_Exit;
    QAction *action_About;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QSplitter *mainSplitter;
    QWidget *leftWidget;
    QVBoxLayout *leftWidgetLayout;
    QSplitter *leftSplitter;
    QWidget *plotWidget;
    QVBoxLayout *plotWidgetLayout;
    QGridLayout *plotLayout;
    QWidget *dashboardsWidget;
    QVBoxLayout *dashboardsWidgetLayout;
    QHBoxLayout *dashboardsLayout;
    QGridLayout *dash1Layout;
    QGridLayout *dash2Layout;
    QGridLayout *dash3Layout;
    QGridLayout *dash4Layout;
    QWidget *rightWidget;
    QVBoxLayout *rightWidgetLayout;
    QSplitter *rightSplitter;
    QWidget *instrumentWidget;
    QVBoxLayout *instrumentWidgetLayout;
    QGridLayout *instrumentLayout;
    QWidget *optionWidget;
    QVBoxLayout *optionWidgetLayout;
    QGridLayout *optionLayout;
    QMenuBar *menubar;
    QMenu *menu_File;
    QMenu *menu_Help;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1920, 1080);
        MainWindow->setMinimumSize(QSize(1920, 1080));
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(16);
        sizePolicy.setVerticalStretch(9);
        sizePolicy.setHeightForWidth(MainWindow->sizePolicy().hasHeightForWidth());
        MainWindow->setSizePolicy(sizePolicy);
        action_Exit = new QAction(MainWindow);
        action_Exit->setObjectName("action_Exit");
        action_About = new QAction(MainWindow);
        action_About->setObjectName("action_About");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setSpacing(4);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(4, 4, 4, 4);
        mainSplitter = new QSplitter(centralwidget);
        mainSplitter->setObjectName("mainSplitter");
        mainSplitter->setOrientation(Qt::Horizontal);
        mainSplitter->setChildrenCollapsible(false);
        mainSplitter->setHandleWidth(0);
        leftWidget = new QWidget(mainSplitter);
        leftWidget->setObjectName("leftWidget");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        sizePolicy1.setHorizontalStretch(1);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(leftWidget->sizePolicy().hasHeightForWidth());
        leftWidget->setSizePolicy(sizePolicy1);
        leftWidget->setMinimumSize(QSize(960, 0));
        leftWidgetLayout = new QVBoxLayout(leftWidget);
        leftWidgetLayout->setSpacing(4);
        leftWidgetLayout->setObjectName("leftWidgetLayout");
        leftWidgetLayout->setContentsMargins(0, 0, 0, 0);
        leftSplitter = new QSplitter(leftWidget);
        leftSplitter->setObjectName("leftSplitter");
        leftSplitter->setOrientation(Qt::Vertical);
        leftSplitter->setChildrenCollapsible(false);
        leftSplitter->setHandleWidth(0);
        plotWidget = new QWidget(leftSplitter);
        plotWidget->setObjectName("plotWidget");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(7);
        sizePolicy2.setHeightForWidth(plotWidget->sizePolicy().hasHeightForWidth());
        plotWidget->setSizePolicy(sizePolicy2);
        plotWidget->setMinimumSize(QSize(0, 770));
        plotWidgetLayout = new QVBoxLayout(plotWidget);
        plotWidgetLayout->setSpacing(0);
        plotWidgetLayout->setObjectName("plotWidgetLayout");
        plotWidgetLayout->setContentsMargins(0, 0, 0, 0);
        plotLayout = new QGridLayout();
        plotLayout->setObjectName("plotLayout");

        plotWidgetLayout->addLayout(plotLayout);

        leftSplitter->addWidget(plotWidget);
        dashboardsWidget = new QWidget(leftSplitter);
        dashboardsWidget->setObjectName("dashboardsWidget");
        QSizePolicy sizePolicy3(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(2);
        sizePolicy3.setHeightForWidth(dashboardsWidget->sizePolicy().hasHeightForWidth());
        dashboardsWidget->setSizePolicy(sizePolicy3);
        dashboardsWidget->setMinimumSize(QSize(0, 220));
        dashboardsWidget->setMaximumSize(QSize(16777215, 220));
        dashboardsWidgetLayout = new QVBoxLayout(dashboardsWidget);
        dashboardsWidgetLayout->setSpacing(0);
        dashboardsWidgetLayout->setObjectName("dashboardsWidgetLayout");
        dashboardsWidgetLayout->setContentsMargins(0, 0, 0, 0);
        dashboardsLayout = new QHBoxLayout();
        dashboardsLayout->setObjectName("dashboardsLayout");
        dash1Layout = new QGridLayout();
        dash1Layout->setObjectName("dash1Layout");

        dashboardsLayout->addLayout(dash1Layout);

        dash2Layout = new QGridLayout();
        dash2Layout->setObjectName("dash2Layout");

        dashboardsLayout->addLayout(dash2Layout);

        dash3Layout = new QGridLayout();
        dash3Layout->setObjectName("dash3Layout");

        dashboardsLayout->addLayout(dash3Layout);

        dash4Layout = new QGridLayout();
        dash4Layout->setObjectName("dash4Layout");

        dashboardsLayout->addLayout(dash4Layout);

        dashboardsLayout->setStretch(0, 1);
        dashboardsLayout->setStretch(1, 1);
        dashboardsLayout->setStretch(2, 1);
        dashboardsLayout->setStretch(3, 1);

        dashboardsWidgetLayout->addLayout(dashboardsLayout);

        leftSplitter->addWidget(dashboardsWidget);

        leftWidgetLayout->addWidget(leftSplitter);

        mainSplitter->addWidget(leftWidget);
        rightWidget = new QWidget(mainSplitter);
        rightWidget->setObjectName("rightWidget");
        sizePolicy1.setHeightForWidth(rightWidget->sizePolicy().hasHeightForWidth());
        rightWidget->setSizePolicy(sizePolicy1);
        rightWidget->setMinimumSize(QSize(960, 0));
        rightWidgetLayout = new QVBoxLayout(rightWidget);
        rightWidgetLayout->setSpacing(4);
        rightWidgetLayout->setObjectName("rightWidgetLayout");
        rightWidgetLayout->setContentsMargins(0, 0, 0, 0);
        rightSplitter = new QSplitter(rightWidget);
        rightSplitter->setObjectName("rightSplitter");
        rightSplitter->setOrientation(Qt::Vertical);
        rightSplitter->setChildrenCollapsible(false);
        rightSplitter->setHandleWidth(0);
        instrumentWidget = new QWidget(rightSplitter);
        instrumentWidget->setObjectName("instrumentWidget");
        sizePolicy2.setHeightForWidth(instrumentWidget->sizePolicy().hasHeightForWidth());
        instrumentWidget->setSizePolicy(sizePolicy2);
        instrumentWidget->setMinimumSize(QSize(0, 770));
        instrumentWidgetLayout = new QVBoxLayout(instrumentWidget);
        instrumentWidgetLayout->setSpacing(0);
        instrumentWidgetLayout->setObjectName("instrumentWidgetLayout");
        instrumentWidgetLayout->setContentsMargins(0, 0, 0, 0);
        instrumentLayout = new QGridLayout();
        instrumentLayout->setObjectName("instrumentLayout");

        instrumentWidgetLayout->addLayout(instrumentLayout);

        rightSplitter->addWidget(instrumentWidget);
        optionWidget = new QWidget(rightSplitter);
        optionWidget->setObjectName("optionWidget");
        sizePolicy3.setHeightForWidth(optionWidget->sizePolicy().hasHeightForWidth());
        optionWidget->setSizePolicy(sizePolicy3);
        optionWidget->setMinimumSize(QSize(0, 220));
        optionWidget->setMaximumSize(QSize(16777215, 220));
        optionWidgetLayout = new QVBoxLayout(optionWidget);
        optionWidgetLayout->setSpacing(0);
        optionWidgetLayout->setObjectName("optionWidgetLayout");
        optionWidgetLayout->setContentsMargins(0, 0, 0, 0);
        optionLayout = new QGridLayout();
        optionLayout->setObjectName("optionLayout");

        optionWidgetLayout->addLayout(optionLayout);

        rightSplitter->addWidget(optionWidget);

        rightWidgetLayout->addWidget(rightSplitter);

        mainSplitter->addWidget(rightWidget);

        verticalLayout->addWidget(mainSplitter);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1920, 18));
        menu_File = new QMenu(menubar);
        menu_File->setObjectName("menu_File");
        menu_Help = new QMenu(menubar);
        menu_Help->setObjectName("menu_Help");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menu_File->menuAction());
        menubar->addAction(menu_Help->menuAction());
        menu_File->addAction(action_Exit);
        menu_Help->addAction(action_About);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "\346\225\260\346\215\256\351\207\207\351\233\206\347\263\273\347\273\237", nullptr));
        action_Exit->setText(QCoreApplication::translate("MainWindow", "\351\200\200\345\207\272", nullptr));
        action_About->setText(QCoreApplication::translate("MainWindow", "\345\205\263\344\272\216", nullptr));
        menu_File->setTitle(QCoreApplication::translate("MainWindow", "\346\226\207\344\273\266", nullptr));
        menu_Help->setTitle(QCoreApplication::translate("MainWindow", "\345\270\256\345\212\251", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
