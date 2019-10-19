#include "scmp_rescale_window.h"

#include "scmp/scmp.h"

#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qmessagebox.h>
#include <qstandardpaths.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>


ScmpRescaleWindow::ScmpRescaleWindow(QWidget *parent):
    QMainWindow(parent)
{
    ui.setupUi(this);

    for (auto pair : {
        std::make_pair("1.25 km (64px)",64),
        std::make_pair("2.5 km (128px)", 128),
        std::make_pair("5 km (256px)", 256),
        std::make_pair("10 km (512px)", 512),
        std::make_pair("20 km (1024px)", 1024),
        std::make_pair("40 km (2048px)", 2048),
        std::make_pair("80 km (4096px)", 4096)
        })
    {
        ui.sourceNewWidthComboBox->addItem(pair.first, QVariant(pair.second));
        ui.sourceNewHeightComboBox->addItem(pair.first, QVariant(pair.second));
        ui.sourceNewWidthComboBox->setCurrentIndex(3);
        ui.sourceNewHeightComboBox->setCurrentIndex(3);
    }

    updateSourceMapInfo();
    updateTargetMapInfo();
    updateSaveOptions();
}


void ScmpRescaleWindow::updateSourceMapInfo()
{
    ui.sourceSizeLabel->setHidden(!m_sourceScmp);
    ui.sourceVersionLabel->setHidden(!m_sourceScmp);
    ui.sourceLayerCountLabel->setHidden(!m_sourceScmp);

    if (m_sourceScmp)
    {
        ui.sourceSizeLabel->setText(QString::number(m_sourceScmp->width) + " x " + QString::number(m_sourceScmp->height) + " (pixels)");
        ui.sourceVersionLabel->setText("map version: "+QString::number(m_sourceScmp->versionMajor) + "." + QString::number(m_sourceScmp->versionMinor));
        ui.sourceLayerCountLabel->setText("strata count: " + QString::number(m_sourceScmp->stratumCount));
    }
}


void ScmpRescaleWindow::updateTargetMapInfo()
{
    ui.targetSizeLabel->setHidden(!m_targetScmp);
    ui.targetVersionLabel->setHidden(!m_targetScmp);
    ui.targetLayerCountLabel->setHidden(!m_targetScmp);

    if (m_targetScmp)
    {
        ui.targetSizeLabel->setText(QString::number(m_targetScmp->width) + " x " + QString::number(m_targetScmp->height) + " (pixels)");
        ui.targetVersionLabel->setText("map version: " + QString::number(m_targetScmp->versionMajor) + "." + QString::number(m_targetScmp->versionMinor));
        ui.targetLayerCountLabel->setText("strata count: " + QString::number(m_targetScmp->stratumCount));
    }
}


void ScmpRescaleWindow::updateSaveOptions()
{
    if (isMergeModeSelected())
    {
        ui.goButton->setEnabled(m_sourceScmp && m_targetScmp);
        ui.mergePositionFrame->setHidden(false);
    }
    else
    {
        ui.goButton->setEnabled(m_sourceScmp && !getTargetFilename().isEmpty());
        ui.mergePositionFrame->setHidden(true);
    }
}


void ScmpRescaleWindow::updatePositionSliders()
{
    if (m_targetScmp)
    {
        ui.sourceHorizontalPositionSpinBox->setMinimum(-getNewSourceWidth());
        ui.sourceHorizontalPositionSpinBox->setMaximum(m_targetScmp->width);

        ui.sourceHorizontalLeftPositionSlider->setMinimum(0);
        ui.sourceHorizontalLeftPositionSlider->setMaximum(m_targetScmp->width);
        ui.sourceHorizontalLeftPositionSlider->setValue(ui.sourceHorizontalPositionSpinBox->value());

        ui.sourceHorizontalRightPositionSlider->setMinimum(0);
        ui.sourceHorizontalRightPositionSlider->setMaximum(m_targetScmp->width);
        ui.sourceHorizontalRightPositionSlider->setValue(ui.sourceHorizontalPositionSpinBox->value() + getNewSourceWidth());

        ui.sourceVerticalPositionSpinBox->setMinimum(-getNewSourceHeight());
        ui.sourceVerticalPositionSpinBox->setMaximum(m_targetScmp->height);

        ui.sourceVerticalTopPositionSlider->setMinimum(0);
        ui.sourceVerticalTopPositionSlider->setMaximum(m_targetScmp->height);
        ui.sourceVerticalTopPositionSlider->setValue(ui.sourceVerticalPositionSpinBox->value());

        ui.sourceVerticalBottomPositionSlider->setMinimum(0);
        ui.sourceVerticalBottomPositionSlider->setMaximum(m_targetScmp->height);
        ui.sourceVerticalBottomPositionSlider->setValue(ui.sourceVerticalPositionSpinBox->value() + getNewSourceHeight());
    }
}


void ScmpRescaleWindow::on_goButton_clicked()
{
    QFileInfo checkFile(getTargetFilename());
    if (checkFile.exists() && checkFile.isFile())
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "File exists.  Overwrite?", getTargetFilename(), QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }

        auto MakeBackupFilename = [this](int n)
        { 
            QString result = this->getTargetFilename() + ".bak";
            if (n > 0)
            {
                result += "(" + QString::number(n) + ")";
            }
            return result;
        };

        QString backupFilename = MakeBackupFilename(0);
        for (int n = 0; QFileInfo(backupFilename).exists(); backupFilename = MakeBackupFilename(++n));
        QFile::copy(getTargetFilename(), backupFilename);
    }

    try
    {
        if (isMergeModeSelected() && m_sourceScmp && m_targetScmp)
        {
            m_sourceScmp->Resize(getNewSourceWidth(), getNewSourceHeight());
            m_targetScmp->Import(*m_sourceScmp, getHorzPosition(), getVertPosition(), isAdditiveMerge());
            std::ofstream ofs(getTargetFilename().toLatin1().data(), std::ios::binary);
            m_targetScmp->Save(ofs);
            QMessageBox::information(this,
                "Rescale/import", "Finished rescaling and importing .scmap.\n"
                "This tool only edits the .scmap file.  Your next step is:\n"
                "- use a map editor to place markers from the imported map\n"
                "  (eg mexes and starting positions)", QMessageBox::Ok);
        }
        else if (!isMergeModeSelected() && m_sourceScmp)
        {
            int newWidthHeight = std::max(getNewSourceWidth(), getNewSourceHeight());
            m_sourceScmp->Resize(newWidthHeight, newWidthHeight);
            std::ofstream ofs(getTargetFilename().toLatin1().data(), std::ios::binary);
            m_sourceScmp->Save(ofs);
            QMessageBox::information(this, "Rescale", 
                "Finished rescaling .scmap.\n"
                "This tool only edits the .scmap file.  Your next steps are:\n"
                "- Edit the _scenario.lua file to set the new map width.\n"
                "- Rename your new .scmap file (or edit the _scenario.lua\n"
                "  file to point to your new .scmap file)\n"
                "- Use a map editor (or edit the _save.lua file) to move\n"
                "  your markers (eg mexes and starting positions)\n"
                "  and adjust your map bounds / areas", QMessageBox::Ok);
        }

    }
    catch (const std::exception & e)
    {
        QMessageBox::information(this, "Error", e.what(), QMessageBox::Ok);
    }

    m_sourceScmp = tryLoadScmp(getSourceFilename());
    updateSourceMapInfo();

    m_targetScmp = tryLoadScmp(getTargetFilename());
    updateTargetMapInfo();
}


void ScmpRescaleWindow::on_exitButton_clicked()
{
    close();
}


void ScmpRescaleWindow::on_sourceNewWidthComboBox_currentIndexChanged(int)
{
    updatePositionSliders();
}


void ScmpRescaleWindow::on_sourceNewHeightComboBox_currentIndexChanged(int)
{
    updatePositionSliders();
}


void ScmpRescaleWindow::on_mergeModeRadioButton_toggled(bool checked)
{
    updateSaveOptions();
}


void ScmpRescaleWindow::on_sourceHorizontalPositionSpinBox_valueChanged(int)
{
    updatePositionSliders();
}


void ScmpRescaleWindow::on_sourceVerticalPositionSpinBox_valueChanged(int)
{
    updatePositionSliders();
}


void ScmpRescaleWindow::on_sourceHorizontalLeftPositionSlider_valueChanged(int v)
{
    ui.sourceHorizontalPositionSpinBox->setValue(v);
}

void ScmpRescaleWindow::on_sourceHorizontalRightPositionSlider_valueChanged(int v)
{
    ui.sourceHorizontalPositionSpinBox->setValue(v - getNewSourceWidth());
}

void ScmpRescaleWindow::on_sourceVerticalTopPositionSlider_valueChanged(int v)
{
    ui.sourceVerticalPositionSpinBox->setValue(v);
}

void ScmpRescaleWindow::on_sourceVerticalBottomPositionSlider_valueChanged(int v)
{
    ui.sourceVerticalPositionSpinBox->setValue(v - getNewSourceHeight());
}


void ScmpRescaleWindow::on_sourceMapButton_clicked()
{
    QString userDocsPath = QStandardPaths::locate(QStandardPaths::StandardLocation::DocumentsLocation, "My Games", QStandardPaths::LocateDirectory);
    QString fn = QFileDialog::getOpenFileName(this, "Source Map filename", userDocsPath, "*.scmap");

    if (!fn.isEmpty())
    {
        ui.sourceMapLineEdit->setText(fn);
    }
}


std::shared_ptr<nfa::scmp::Scmp>  ScmpRescaleWindow::tryLoadScmp(const QString &fn)
{
    std::shared_ptr<nfa::scmp::Scmp> scmp;

    std::ifstream ifs((const char*)fn.toLatin1().data(), std::ios::binary);
    if (!ifs.good())
    {
        return scmp;
    }

    try
    {
        scmp.reset(new nfa::scmp::Scmp(ifs));
        scmp->MapInfo(std::cout);
    }
    catch (std::exception &e)
    {
        std::ostringstream ss;
        ss << "Unable to parse " << fn.toStdString() << ":" << e.what();
        QMessageBox messagebox;
        messagebox.critical(0, "Error", QString::fromStdString(ss.str()));
    }
    return scmp;
}


void ScmpRescaleWindow::on_sourceMapLineEdit_textChanged(const QString &fn)
{
    m_sourceScmp = tryLoadScmp(fn);
    updateSourceMapInfo();
    updatePositionSliders();
    updateSaveOptions();
}


void ScmpRescaleWindow::on_targetMapButton_clicked()
{
    QString userDocsPath = QStandardPaths::locate(QStandardPaths::StandardLocation::DocumentsLocation, "My Games", QStandardPaths::LocateDirectory);
    QString fn = QFileDialog::getOpenFileName(this, "Target Map filename", userDocsPath, "*.scmap");

    if (!fn.isEmpty())
    {
        ui.targetMapLineEdit->setText(fn);
    }
}


void ScmpRescaleWindow::on_targetMapLineEdit_textChanged(const QString &fn)
{
    m_targetScmp = tryLoadScmp(fn);
    updateTargetMapInfo();
    updatePositionSliders();
    updateSaveOptions();
}

std::shared_ptr<nfa::scmp::Scmp> ScmpRescaleWindow::getSourceScmp()
{
    return m_sourceScmp;
}

std::shared_ptr<nfa::scmp::Scmp> ScmpRescaleWindow::getTargetScmp()
{
    return m_targetScmp;
}

QString ScmpRescaleWindow::getSourceFilename()
{
    return ui.sourceMapLineEdit->text();
}

QString ScmpRescaleWindow::getTargetFilename()
{
    return ui.targetMapLineEdit->text();
}

int ScmpRescaleWindow::getNewSourceWidth()
{
    return ui.sourceNewWidthComboBox->currentData().toInt();
}

int ScmpRescaleWindow::getNewSourceHeight()
{
    return ui.sourceNewHeightComboBox->currentData().toInt();
}

int ScmpRescaleWindow::getHorzPosition()
{
    return ui.sourceHorizontalPositionSpinBox->value();
}

int ScmpRescaleWindow::getVertPosition()
{
    return ui.sourceVerticalPositionSpinBox->value();
}

bool ScmpRescaleWindow::isMergeModeSelected()
{
    return ui.mergeModeRadioButton->isChecked();
}

bool ScmpRescaleWindow::isAdditiveMerge()
{
    return ui.additiveMergeCheckBox->isChecked();
}