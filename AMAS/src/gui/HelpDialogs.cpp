#include "HelpDialogs.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextBrowser>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>

namespace AMAS {

// ── AboutDialog Implementation ──────────────────────────────────────
AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About AMAS"));
    resize(420, 320);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    auto *lblTitle = new QLabel(tr("Antenna Measurement & Analysis Software"), this);
    lblTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #007ACC;");
    lblTitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(lblTitle);

    auto *lblVer = new QLabel(tr("Version 1.0.0 (Production Build)"), this);
    lblVer->setStyleSheet("font-size: 12px; color: #808080;");
    lblVer->setAlignment(Qt::AlignCenter);
    layout->addWidget(lblVer);

    auto *desc = new QLabel(this);
    desc->setWordWrap(true);
    desc->setStyleSheet("font-size: 13px; line-height: 18px;");
    desc->setText(tr("AMAS is a professional, industrial desktop software package "
                     "engineered for automated RF antenna parameter sweeps, coaxial calibration "
                     "interpolation, real-time Cartesian and polar radiation pattern extraction, "
                     "and engineering report formatting."));
    layout->addWidget(desc);

    auto *credits = new QLabel(this);
    credits->setWordWrap(true);
    credits->setStyleSheet("font-size: 12px; color: #C8C8C8;");
    credits->setText(tr("<b>Developer Credits:</b> Advanced Agentic Coding Team, Google DeepMind.<br>"
                        "<b>License:</b> Proprietary Industrial Application License (Commercial Use)."));
    layout->addWidget(credits);

    auto *btnBox = new QHBoxLayout();
    btnBox->addStretch();
    auto *btnOk = new QPushButton(tr("OK"), this);
    btnOk->setMinimumWidth(100);
    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    btnBox->addWidget(btnOk);
    layout->addLayout(btnBox);
}

// ── UserManualDialog Implementation ──────────────────────────────────
UserManualDialog::UserManualDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("AMAS User Manual"));
    resize(640, 500);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *browser = new QTextBrowser(this);
    browser->setStyleSheet("background-color: #1E1E1E; color: #FFFFFF; font-family: 'Segoe UI', sans-serif;");
    browser->setHtml(
        "<h2>AMAS Quick Start User Manual</h2>"
        "<p>Welcome to AMAS (Antenna Measurement & Analysis Software). This guide outlines the standard coordinated testing workflow.</p>"
        "<h3>1. Hardware Connection Setup</h3>"
        "<ul>"
        "  <li>Navigate to the <b>Devices</b> page (Ctrl+2).</li>"
        "  <li>Select VNA interface resource (VISA TCPIP/GPIB) and rotation table COM port.</li>"
        "  <li>Click <b>Connect</b>. Confirm status indicators display <i>Connected</i> (Green).</li>"
        "</ul>"
        "<h3>2. Sweep Configurations & Profile Profiles</h3>"
        "<ul>"
        "  <li>Navigate to the <b>Measurement Setup</b> page (Ctrl+4).</li>"
        "  <li>Set the desired Frequency range, points count, and IF Bandwidth.</li>"
        "  <li>Select <i>Use Rotation Table</i> for angular sweeps, specifying step resolution (e.g. 5 or 10 deg).</li>"
        "  <li>Click <b>Save Profile</b> to register the setup configuration.</li>"
        "</ul>"
        "<h3>3. Performing Calibration</h3>"
        "<ul>"
        "  <li>Click <b>Load Calibration</b>. Choose a valid chamber calibration short shim/SOLT state matching your bandwidth limits.</li>"
        "</ul>"
        "<h3>4. Acquiring Sweeps</h3>"
        "<ul>"
        "  <li>Go to <b>Progress Monitor</b> (Ctrl+5) or click <b>Start Sweep</b> (F5).</li>"
        "  <li>Confirm visual marker values update as VNA captures magnitude and phase parameters.</li>"
        "</ul>"
        "<h3>5. Analysis & Report Bundles</h3>"
        "<ul>"
        "  <li>Navigate to the <b>Results Page</b> (Ctrl+6). Add comments or operators in the metadata edit fields.</li>"
        "  <li>Click <b>Generate Report Bundle</b>. This exports a structured A4 PDF, HTML template, statistics.csv, and layout PNGs inside the <i>Reports/</i> folder.</li>"
        "</ul>"
    );
    layout->addWidget(browser);

    auto *btnBox = new QHBoxLayout();
    btnBox->addStretch();
    auto *btnOk = new QPushButton(tr("Close"), this);
    btnOk->setMinimumWidth(100);
    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    btnBox->addWidget(btnOk);
    layout->addLayout(btnBox);
}

// ── ShortcutsDialog Implementation ───────────────────────────────────
ShortcutsDialog::ShortcutsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Keyboard Shortcuts"));
    resize(500, 360);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *lblTitle = new QLabel(tr("AMAS Application Keyboard Shortcuts"), this);
    lblTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #007ACC;");
    layout->addWidget(lblTitle);

    auto *table = new QTableWidget(11, 2, this);
    table->setHorizontalHeaderLabels({tr("Shortcut"), tr("Description")});
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setStyleSheet("QTableWidget { background-color: #1E1E1E; border: 1px solid #3F3F46; } QHeaderView::section { background-color: #2D2D30; color: #FFFFFF; }");

    auto setRow = [table](int r, const QString &key, const QString &desc) {
        auto *itemK = new QTableWidgetItem(key);
        itemK->setForeground(QBrush(QColor("#00E5FF")));
        itemK->setFont(QFont("Consolas", 11, QFont::Bold));
        table->setItem(r, 0, itemK);

        auto *itemD = new QTableWidgetItem(desc);
        itemD->setForeground(QBrush(QColor("#FFFFFF")));
        table->setItem(r, 1, itemD);
    };

    setRow(0, "Ctrl+N", tr("Create a new measurement profile setup."));
    setRow(1, "Ctrl+O", tr("Open an existing profile setup file."));
    setRow(2, "Ctrl+S", tr("Save the active measurement profile setup."));
    setRow(3, "Ctrl+Shift+S", tr("Save active setup profile as a new filename."));
    setRow(4, "Ctrl+Q", tr("Close the application gracefully."));
    setRow(5, "Ctrl+Z", tr("Undo last configuration/metadata edit action."));
    setRow(6, "Ctrl+Y", tr("Redo last undone action."));
    setRow(7, "Ctrl+1 - Ctrl+6", tr("Switch between main navigation page indices (Dashboard, Devices, Setup, Progress, Results, Profiles)."));
    setRow(8, "F5", tr("Start/Trigger the active measurement sweep coordinate run."));
    setRow(9, "F9", tr("Validate current setup configuration inputs limits."));
    setRow(10, "Ctrl+R", tr("Refresh connected devices status."));

    layout->addWidget(table);

    auto *btnBox = new QHBoxLayout();
    btnBox->addStretch();
    auto *btnOk = new QPushButton(tr("Close"), this);
    btnOk->setMinimumWidth(100);
    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    btnBox->addWidget(btnOk);
    layout->addLayout(btnBox);
}

} // namespace AMAS
