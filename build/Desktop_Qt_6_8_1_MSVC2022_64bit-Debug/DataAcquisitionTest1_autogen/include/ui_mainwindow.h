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
#include <QtWidgets/QGroupBox>
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
    QGroupBox *plotGroupBox;
    QVBoxLayout *plotGroupBoxLayout;
    QWidget *dashboardsWidget;
    QVBoxLayout *dashboardsWidgetLayout;
    QHBoxLayout *dashboardsLayout;
    QGroupBox *dashboardGroupBox;
    QHBoxLayout *dashboardGroupBoxLayout;
    QWidget *dashWidget1;
    QWidget *dashWidget2;
    QWidget *dashWidget3;
    QWidget *dashWidget4;
    QWidget *rightWidget;
    QVBoxLayout *rightWidgetLayout;
    QSplitter *rightSplitter;
    QWidget *instrumentWidget;
    QVBoxLayout *instrumentWidgetLayout;
    QGridLayout *instrumentLayout;
    QGroupBox *instrumentGroupBox;
    QVBoxLayout *instrumentGroupBoxLayout;
    QWidget *optionWidget;
    QVBoxLayout *optionWidgetLayout;
    QGridLayout *optionLayout;
    QGroupBox *optionGroupBox;
    QVBoxLayout *optionGroupBoxLayout;
    QMenuBar *menubar;
    QMenu *menu_File;
    QMenu *menu_Help;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1920, 1080);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(16);
        sizePolicy.setVerticalStretch(9);
        sizePolicy.setHeightForWidth(MainWindow->sizePolicy().hasHeightForWidth());
        MainWindow->setSizePolicy(sizePolicy);
        MainWindow->setMinimumSize(QSize(1920, 1080));
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
        mainSplitter->setOrientation(Qt::Orientation::Horizontal);
        mainSplitter->setHandleWidth(0);
        mainSplitter->setChildrenCollapsible(false);
        leftWidget = new QWidget(mainSplitter);
        leftWidget->setObjectName("leftWidget");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        sizePolicy1.setHorizontalStretch(1);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(leftWidget->sizePolicy().hasHeightForWidth());
        leftWidget->setSizePolicy(sizePolicy1);
        leftWidget->setMinimumSize(QSize(960, 0));
        leftWidgetLayout = new QVBoxLayout(leftWidget);
        leftWidgetLayout->setSpacing(8);
        leftWidgetLayout->setObjectName("leftWidgetLayout");
        leftWidgetLayout->setContentsMargins(4, 4, 4, 4);
        leftSplitter = new QSplitter(leftWidget);
        leftSplitter->setObjectName("leftSplitter");
        leftSplitter->setOrientation(Qt::Orientation::Vertical);
        leftSplitter->setHandleWidth(0);
        leftSplitter->setChildrenCollapsible(false);
        plotWidget = new QWidget(leftSplitter);
        plotWidget->setObjectName("plotWidget");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(3);
        sizePolicy2.setHeightForWidth(plotWidget->sizePolicy().hasHeightForWidth());
        plotWidget->setSizePolicy(sizePolicy2);
        plotWidget->setMinimumSize(QSize(0, 750));
        plotWidgetLayout = new QVBoxLayout(plotWidget);
        plotWidgetLayout->setSpacing(0);
        plotWidgetLayout->setObjectName("plotWidgetLayout");
        plotWidgetLayout->setContentsMargins(0, 0, 0, 0);
        plotLayout = new QGridLayout();
        plotLayout->setObjectName("plotLayout");
        plotGroupBox = new QGroupBox(plotWidget);
        plotGroupBox->setObjectName("plotGroupBox");
        plotGroupBox->setStyleSheet(QString::fromUtf8("QGroupBox {\n"
"    background-color: white;\n"
"    border: 1px solid #cccccc;\n"
"    border-radius: 8px;\n"
"    margin-top: 8px;\n"
"    font-weight: bold;\n"
"}\n"
"QGroupBox::title {\n"
"    subcontrol-origin: margin;\n"
"    left: 10px;\n"
"    padding: 0 3px 0 3px;\n"
"}"));
        plotGroupBoxLayout = new QVBoxLayout(plotGroupBox);
        plotGroupBoxLayout->setObjectName("plotGroupBoxLayout");
        plotGroupBoxLayout->setContentsMargins(8, 16, 8, 8);

        plotLayout->addWidget(plotGroupBox, 0, 0, 1, 1);


        plotWidgetLayout->addLayout(plotLayout);

        leftSplitter->addWidget(plotWidget);
        dashboardsWidget = new QWidget(leftSplitter);
        dashboardsWidget->setObjectName("dashboardsWidget");
        QSizePolicy sizePolicy3(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(1);
        sizePolicy3.setHeightForWidth(dashboardsWidget->sizePolicy().hasHeightForWidth());
        dashboardsWidget->setSizePolicy(sizePolicy3);
        dashboardsWidget->setMinimumSize(QSize(0, 250));
        dashboardsWidget->setMaximumSize(QSize(16777215, 250));
        dashboardsWidgetLayout = new QVBoxLayout(dashboardsWidget);
        dashboardsWidgetLayout->setSpacing(0);
        dashboardsWidgetLayout->setObjectName("dashboardsWidgetLayout");
        dashboardsWidgetLayout->setContentsMargins(0, 0, 0, 0);
        dashboardsLayout = new QHBoxLayout();
        dashboardsLayout->setObjectName("dashboardsLayout");
        dashboardGroupBox = new QGroupBox(dashboardsWidget);
        dashboardGroupBox->setObjectName("dashboardGroupBox");
        dashboardGroupBox->setStyleSheet(QString::fromUtf8("QGroupBox {\n"
"    background-color: white;\n"
"    border: 1px solid #cccccc;\n"
"    border-radius: 8px;\n"
"    margin-top: 8px;\n"
"    font-weight: bold;\n"
"}\n"
"QGroupBox::title {\n"
"    subcontrol-origin: margin;\n"
"    left: 10px;\n"
"    padding: 0 3px 0 3px;\n"
"}"));
        dashboardGroupBoxLayout = new QHBoxLayout(dashboardGroupBox);
        dashboardGroupBoxLayout->setSpacing(8);
        dashboardGroupBoxLayout->setObjectName("dashboardGroupBoxLayout");
        dashboardGroupBoxLayout->setContentsMargins(8, 16, 8, 8);
        dashWidget1 = new QWidget(dashboardGroupBox);
        dashWidget1->setObjectName("dashWidget1");
        dashWidget1->setMinimumSize(QSize(0, 0));
        dashWidget1->setStyleSheet(QString::fromUtf8("background-color: #f5f5f5;\n"
"border-radius: 4px;"));

        dashboardGroupBoxLayout->addWidget(dashWidget1);

        dashWidget2 = new QWidget(dashboardGroupBox);
        dashWidget2->setObjectName("dashWidget2");
        dashWidget2->setStyleSheet(QString::fromUtf8("background-color: #f5f5f5;\n"
"border-radius: 4px;"));

        dashboardGroupBoxLayout->addWidget(dashWidget2);

        dashWidget3 = new QWidget(dashboardGroupBox);
        dashWidget3->setObjectName("dashWidget3");
        dashWidget3->setStyleSheet(QString::fromUtf8("background-color: #f5f5f5;\n"
"border-radius: 4px;"));

        dashboardGroupBoxLayout->addWidget(dashWidget3);

        dashWidget4 = new QWidget(dashboardGroupBox);
        dashWidget4->setObjectName("dashWidget4");
        dashWidget4->setStyleSheet(QString::fromUtf8("background-color: #f5f5f5;\n"
"border-radius: 4px;"));

        dashboardGroupBoxLayout->addWidget(dashWidget4);

        dashboardGroupBoxLayout->setStretch(0, 1);
        dashboardGroupBoxLayout->setStretch(1, 1);
        dashboardGroupBoxLayout->setStretch(2, 1);
        dashboardGroupBoxLayout->setStretch(3, 1);

        dashboardsLayout->addWidget(dashboardGroupBox);


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
        rightWidgetLayout->setSpacing(8);
        rightWidgetLayout->setObjectName("rightWidgetLayout");
        rightWidgetLayout->setContentsMargins(4, 4, 4, 4);
        rightSplitter = new QSplitter(rightWidget);
        rightSplitter->setObjectName("rightSplitter");
        rightSplitter->setOrientation(Qt::Orientation::Vertical);
        rightSplitter->setHandleWidth(0);
        rightSplitter->setChildrenCollapsible(false);
        instrumentWidget = new QWidget(rightSplitter);
        instrumentWidget->setObjectName("instrumentWidget");
        sizePolicy2.setHeightForWidth(instrumentWidget->sizePolicy().hasHeightForWidth());
        instrumentWidget->setSizePolicy(sizePolicy2);
        instrumentWidget->setMinimumSize(QSize(0, 750));
        instrumentWidgetLayout = new QVBoxLayout(instrumentWidget);
        instrumentWidgetLayout->setSpacing(0);
        instrumentWidgetLayout->setObjectName("instrumentWidgetLayout");
        instrumentWidgetLayout->setContentsMargins(0, 0, 0, 0);
        instrumentLayout = new QGridLayout();
        instrumentLayout->setObjectName("instrumentLayout");
        instrumentGroupBox = new QGroupBox(instrumentWidget);
        instrumentGroupBox->setObjectName("instrumentGroupBox");
        instrumentGroupBox->setStyleSheet(QString::fromUtf8("QGroupBox {\n"
"    background-color: white;\n"
"    border: 1px solid #cccccc;\n"
"    border-radius: 8px;\n"
"    margin-top: 8px;\n"
"    font-weight: bold;\n"
"}\n"
"QGroupBox::title {\n"
"    subcontrol-origin: margin;\n"
"    left: 10px;\n"
"    padding: 0 3px 0 3px;\n"
"}"));
        instrumentGroupBoxLayout = new QVBoxLayout(instrumentGroupBox);
        instrumentGroupBoxLayout->setObjectName("instrumentGroupBoxLayout");
        instrumentGroupBoxLayout->setContentsMargins(8, 16, 8, 8);

        instrumentLayout->addWidget(instrumentGroupBox, 0, 0, 1, 1);


        instrumentWidgetLayout->addLayout(instrumentLayout);

        rightSplitter->addWidget(instrumentWidget);
        optionWidget = new QWidget(rightSplitter);
        optionWidget->setObjectName("optionWidget");
        sizePolicy3.setHeightForWidth(optionWidget->sizePolicy().hasHeightForWidth());
        optionWidget->setSizePolicy(sizePolicy3);
        optionWidget->setMinimumSize(QSize(0, 250));
        optionWidget->setMaximumSize(QSize(16777215, 250));
        optionWidgetLayout = new QVBoxLayout(optionWidget);
        optionWidgetLayout->setSpacing(0);
        optionWidgetLayout->setObjectName("optionWidgetLayout");
        optionWidgetLayout->setContentsMargins(0, 0, 0, 0);
        optionLayout = new QGridLayout();
        optionLayout->setObjectName("optionLayout");
        optionGroupBox = new QGroupBox(optionWidget);
        optionGroupBox->setObjectName("optionGroupBox");
        optionGroupBox->setStyleSheet(QString::fromUtf8("QGroupBox {\n"
"    background-color: white;\n"
"    border: 1px solid #cccccc;\n"
"    border-radius: 8px;\n"
"    margin-top: 8px;\n"
"    font-weight: bold;\n"
"}\n"
"QGroupBox::title {\n"
"    subcontrol-origin: margin;\n"
"    left: 10px;\n"
"    padding: 0 3px 0 3px;\n"
"}"));
        optionGroupBoxLayout = new QVBoxLayout(optionGroupBox);
        optionGroupBoxLayout->setObjectName("optionGroupBoxLayout");
        optionGroupBoxLayout->setContentsMargins(8, 16, 8, 8);

        optionLayout->addWidget(optionGroupBox, 0, 0, 1, 1);


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
        plotGroupBox->setTitle(QCoreApplication::translate("MainWindow", "\347\233\221\346\216\247\346\233\262\347\272\277", nullptr));
        dashboardGroupBox->setTitle(QCoreApplication::translate("MainWindow", "\344\273\252\350\241\250\347\233\230", nullptr));
        instrumentGroupBox->setTitle(QCoreApplication::translate("MainWindow", "\346\225\260\346\215\256\345\217\257\350\247\206\345\214\226", nullptr));
        optionGroupBox->setTitle(QCoreApplication::translate("MainWindow", "\346\223\215\344\275\234\345\214\272", nullptr));
        menu_File->setTitle(QCoreApplication::translate("MainWindow", "\346\226\207\344\273\266", nullptr));
        menu_Help->setTitle(QCoreApplication::translate("MainWindow", "\345\270\256\345\212\251", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
