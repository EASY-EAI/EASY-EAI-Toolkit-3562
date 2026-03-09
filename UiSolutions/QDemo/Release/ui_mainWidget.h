/********************************************************************************
** Form generated from reading UI file 'mainWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWIDGET_H
#define UI_MAINWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Widget
{
public:
    QVBoxLayout *verticalLayout_3;
    QFrame *mpBgFrame;
    QVBoxLayout *verticalLayout;
    QFrame *mpMainPage;
    QFrame *mpBottomBar;
    QVBoxLayout *verticalLayout_2;
    QGridLayout *gridLayout;
    QLabel *mpDateLab;
    QPushButton *mpStandbyBtn;
    QLabel *mpWeekLab;
    QLabel *mpProductLab;
    QSpacerItem *horizontalSpacer;
    QLabel *mpIPLab;
    QLabel *mpWIFILab;
    QLabel *mpTimeLab;
    QSpacerItem *horizontalSpacer_2;
    QHBoxLayout *horizontalLayout;
    QLabel *mpSNLab;

    void setupUi(QWidget *Widget)
    {
        if (Widget->objectName().isEmpty())
            Widget->setObjectName(QString::fromUtf8("Widget"));
        Widget->resize(720, 1080);
        Widget->setMinimumSize(QSize(0, 800));
        Widget->setMaximumSize(QSize(720, 1080));
        Widget->setStyleSheet(QString::fromUtf8("QWidget{\n"
"	border: 0px;	\n"
"	outline: none;\n"
"}"));
        verticalLayout_3 = new QVBoxLayout(Widget);
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setContentsMargins(11, 11, 11, 11);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        mpBgFrame = new QFrame(Widget);
        mpBgFrame->setObjectName(QString::fromUtf8("mpBgFrame"));
        mpBgFrame->setStyleSheet(QString::fromUtf8("QFrame#mpBgFrame{\n"
"	border-image: url(:/image/background.png);\n"
"}"));
        mpBgFrame->setFrameShape(QFrame::StyledPanel);
        mpBgFrame->setFrameShadow(QFrame::Raised);
        verticalLayout = new QVBoxLayout(mpBgFrame);
        verticalLayout->setSpacing(0);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        mpMainPage = new QFrame(mpBgFrame);
        mpMainPage->setObjectName(QString::fromUtf8("mpMainPage"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(mpMainPage->sizePolicy().hasHeightForWidth());
        mpMainPage->setSizePolicy(sizePolicy);
        mpMainPage->setStyleSheet(QString::fromUtf8(""));
        mpMainPage->setFrameShape(QFrame::StyledPanel);
        mpMainPage->setFrameShadow(QFrame::Raised);

        verticalLayout->addWidget(mpMainPage);

        mpBottomBar = new QFrame(mpBgFrame);
        mpBottomBar->setObjectName(QString::fromUtf8("mpBottomBar"));
        mpBottomBar->setMinimumSize(QSize(720, 340));
        mpBottomBar->setMaximumSize(QSize(800, 340));
        mpBottomBar->setStyleSheet(QString::fromUtf8("QFrame#mpBottomBar {\n"
"	border-image: url(:/image/img_bottombar_bg.png);\n"
"}"));
        mpBottomBar->setFrameShape(QFrame::StyledPanel);
        mpBottomBar->setFrameShadow(QFrame::Raised);
        verticalLayout_2 = new QVBoxLayout(mpBottomBar);
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setContentsMargins(11, 11, 11, 11);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(20, 0, 20, 0);
        gridLayout = new QGridLayout();
        gridLayout->setSpacing(0);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, -1, 0, -1);
        mpDateLab = new QLabel(mpBottomBar);
        mpDateLab->setObjectName(QString::fromUtf8("mpDateLab"));
        mpDateLab->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(mpDateLab, 1, 0, 1, 1);

        mpStandbyBtn = new QPushButton(mpBottomBar);
        mpStandbyBtn->setObjectName(QString::fromUtf8("mpStandbyBtn"));
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(mpStandbyBtn->sizePolicy().hasHeightForWidth());
        mpStandbyBtn->setSizePolicy(sizePolicy1);
        QFont font;
        font.setPointSize(16);
        mpStandbyBtn->setFont(font);
        mpStandbyBtn->setStyleSheet(QString::fromUtf8("QPushButton#mpStandbyBtn{\n"
"	border-image: url(:/image/icon_welcom_btn.png);\n"
"}"));

        gridLayout->addWidget(mpStandbyBtn, 0, 1, 4, 1);

        mpWeekLab = new QLabel(mpBottomBar);
        mpWeekLab->setObjectName(QString::fromUtf8("mpWeekLab"));
        mpWeekLab->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(mpWeekLab, 2, 0, 1, 1);

        mpProductLab = new QLabel(mpBottomBar);
        mpProductLab->setObjectName(QString::fromUtf8("mpProductLab"));
        mpProductLab->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(mpProductLab, 3, 0, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 100, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 0, 0, 1, 1);

        mpIPLab = new QLabel(mpBottomBar);
        mpIPLab->setObjectName(QString::fromUtf8("mpIPLab"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(mpIPLab->sizePolicy().hasHeightForWidth());
        mpIPLab->setSizePolicy(sizePolicy2);
        mpIPLab->setMinimumSize(QSize(0, 42));
        mpIPLab->setMaximumSize(QSize(16777215, 42));
        mpIPLab->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(mpIPLab, 3, 2, 1, 1);

        mpWIFILab = new QLabel(mpBottomBar);
        mpWIFILab->setObjectName(QString::fromUtf8("mpWIFILab"));
        QSizePolicy sizePolicy3(QSizePolicy::Fixed, QSizePolicy::Minimum);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(mpWIFILab->sizePolicy().hasHeightForWidth());
        mpWIFILab->setSizePolicy(sizePolicy3);
        mpWIFILab->setMinimumSize(QSize(23, 18));
        mpWIFILab->setStyleSheet(QString::fromUtf8("QLabel#mpWIFILab {\n"
"	image: url(:/image/icon_wifi_disconnect.png);\n"
"}"));
        mpWIFILab->setAlignment(Qt::AlignHCenter|Qt::AlignTop);

        gridLayout->addWidget(mpWIFILab, 3, 3, 1, 1);

        mpTimeLab = new QLabel(mpBottomBar);
        mpTimeLab->setObjectName(QString::fromUtf8("mpTimeLab"));
        mpTimeLab->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(mpTimeLab, 1, 2, 2, 2);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_2, 0, 2, 1, 2);

        gridLayout->setColumnStretch(0, 189);
        gridLayout->setColumnStretch(1, 302);
        gridLayout->setColumnStretch(2, 189);

        verticalLayout_2->addLayout(gridLayout);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(8);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(-1, -1, 0, -1);
        mpSNLab = new QLabel(mpBottomBar);
        mpSNLab->setObjectName(QString::fromUtf8("mpSNLab"));

        horizontalLayout->addWidget(mpSNLab);


        verticalLayout_2->addLayout(horizontalLayout);

        verticalLayout_2->setStretch(0, 7);
        verticalLayout_2->setStretch(1, 1);

        verticalLayout->addWidget(mpBottomBar);


        verticalLayout_3->addWidget(mpBgFrame);


        retranslateUi(Widget);

        QMetaObject::connectSlotsByName(Widget);
    } // setupUi

    void retranslateUi(QWidget *Widget)
    {
        Widget->setWindowTitle(QCoreApplication::translate("Widget", "Widget", nullptr));
        mpDateLab->setText(QCoreApplication::translate("Widget", "2020\345\271\2647\346\234\21027\346\227\245", nullptr));
        mpStandbyBtn->setText(QString());
        mpWeekLab->setText(QCoreApplication::translate("Widget", "\346\230\237\346\234\237\344\270\200", nullptr));
        mpProductLab->setText(QCoreApplication::translate("Widget", "\346\231\272\350\203\275\350\257\206\345\210\253\347\273\210\347\253\257", nullptr));
        mpIPLab->setText(QCoreApplication::translate("Widget", "IP:unknow", nullptr));
        mpWIFILab->setText(QString());
        mpTimeLab->setText(QCoreApplication::translate("Widget", "10:40", nullptr));
        mpSNLab->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class Widget: public Ui_Widget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWIDGET_H
