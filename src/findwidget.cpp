#include <QAction>
#include <QDebug>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

#include "findwidget.h"
#include "mainwindow.h"

extern Main *mainWindow;
extern bool usingDarkTheme;

FindWidget::FindWidget(QWidget *)
{
    QVBoxLayout *mainLayout = new QVBoxLayout;
    QHBoxLayout *row2Layout = new QHBoxLayout;

    QLabel *label = new QLabel;
    label->setText(tr("Find:", "FindWidget"));

    // Create LineEdit (here QComboBox)
    findcombo = new QComboBox;
    findcombo->setMinimumWidth(250);
    findcombo->setEditable(true);

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    findcombo->setSizePolicy(sizePolicy);
    connect(findcombo, SIGNAL(highlighted(int)), this, SLOT(nextPressed()));
    connect(findcombo, SIGNAL(editTextChanged(const QString &)), this,
            SLOT(findTextChanged(const QString &)));

    nextButton = new QPushButton;
    nextButton->setIcon(QPixmap(":/find.png"));
    // nextButton->setText (tr("Find","Find widget"));
    connect(nextButton, SIGNAL(clicked()), this, SLOT(nextPressed()));

    // QAction needed to only activate shortcut while FindWidget has focus
    QAction *a = new QAction(nextButton->text(), this);
    a->setShortcut(Qt::Key_Return);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a, SIGNAL(triggered()), this, SLOT(nextPressed()));
    addAction(a);

    filterNotesButton = new QPushButton;
    filterNotesButton->setIcon(QPixmap(":/flag-note.svg"));
    filterNotesButton->setCheckable(true);
    filterNotesButton->setChecked(true);
    connect(filterNotesButton, SIGNAL(clicked()), this, SLOT(nextPressed()));

    row2Layout->addWidget(label);
    row2Layout->addWidget(findcombo);
    row2Layout->addWidget(nextButton);
    row2Layout->addWidget(filterNotesButton);

    mainLayout->addLayout(row2Layout);

    setLayout(mainLayout);
    status = Undefined;
}

QString FindWidget::getFindText() { return findcombo->currentText(); }

void FindWidget::cancelPressed()
{
    hide();
    emit(hideFindWidget()); // Restore focus
}

void FindWidget::nextPressed()
{
    emit(nextButtonPressed(findcombo->currentText(),
                           filterNotesButton->isChecked()));
}

void FindWidget::findTextChanged(const QString &) { setStatus(Undefined); }

void FindWidget::setFocus()
{
    findcombo->lineEdit()->selectAll();
    findcombo->lineEdit()->setFocus();
}

void FindWidget::setStatus(Status st)
{
    if (st == status)
        return;

    status = st;
    QPalette p = palette();
    QColor c;
    if (usingDarkTheme)
        switch (st) {
            case Success:
                c = QColor(0, 170, 0);
                break;
            case Failed:
                c = QColor(170, 0, 0);
                break;
            default:
                c = p.color(QPalette::Base);
        }
    else
        switch (st) {
            case Success:
                c = QColor(120, 255, 120);
                break;
            case Failed:
                c = QColor(255, 120, 120);
                break;
            default:
                c = p.color(QPalette::Base);
        }
    p.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(9), c);
    p.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(9), c);
    findcombo->setPalette(p);
}
