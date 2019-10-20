#include "scmp_rescale_window.h"

#include "scmp/scmp.h"

#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qmessagebox.h>
#include <qstandardpaths.h>

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <regex>
#include <sstream>


void BackupFile(QString filename)
{
    if (!QFileInfo(filename).exists())
    {
        return;
    }

    auto MakeBackupFilename = [filename](int n)
    {
        QString result = filename + ".bak";
        if (n > 0)
        {
            result += "(" + QString::number(n) + ")";
        }
        return result;
    };

    QString backupFilename = MakeBackupFilename(0);
    for (int n = 0; QFileInfo(backupFilename).exists(); backupFilename = MakeBackupFilename(++n));
    QFile::copy(filename, backupFilename);
}


std::map<QString, QString>  GetMapLuaFileNames(const QString &scmapFilename)
{
    std::map<QString, QString> filenames;

    QFileInfo file_info(scmapFilename);
    QString baseFilename = QDir(file_info.absolutePath()).filePath(file_info.baseName());

    filenames["scmap"] = scmapFilename;
    filenames["save"] = baseFilename + "_save.lua";
    filenames["scenario"] = baseFilename + "_scenario.lua";
    filenames["script"] = baseFilename + "_script.lua";

    for (auto it = filenames.begin(); it != filenames.end(); ++it)
    {
        std::cout << it->first.toStdString() << ": " << it->second.toStdString() << std::endl;
    }

    return filenames;
}


void ModifyLines(std::istream &is, std::ostream &os,
    std::function< void(std::ostream &, std::string &, const std::vector<std::string> &) > f)
{
    std::string line;
    std::regex r("([a-zA-Z]+|[+-]?([0-9]*[.])?[0-9]+)");

    while (!is.eof())
    {
        std::getline(is, line);

        std::vector<std::string> keywords;
        for (auto it = std::sregex_iterator(line.begin(), line.end(), r); it != std::sregex_iterator(); ++it)
        {
            keywords.push_back(it->str());
        }
        f(os, line, keywords);
    }
}


void RescaleMapSaveFile(
    QString sourceFilename, QString targetFilename,
    double xscale, double zscale, double xofs, double zofs, nfa::scmp::Scmp *scmp)
{
    if (!QFileInfo(sourceFilename).exists())
    {
        return;
    }

    std::ostringstream oss;
    {
        std::ifstream ifs(sourceFilename.toLatin1().data());

        ModifyLines(ifs, oss, [xscale, zscale, xofs, zofs, scmp](std::ostream &s, std::string &line, const std::vector<std::string> &kws)
        {
            if (kws.size() == 6 && kws[0] == "position" && kws[1] == "VECTOR" && kws[2] == "3")
            {
                double x = std::atof(kws[3].c_str()) * xscale + xofs;
                double z = std::atof(kws[5].c_str()) * zscale + zofs;
                double y = scmp->heightScale * scmp->HeightMapAt(x, z);
                s << "['position'] = VECTOR3( " << x << ", " << y << ", " << z << " ),\n";
            }
            else if (kws.size() == 6 && kws[0] == "rectangle" && kws[1] == "RECTANGLE")
            {
                double xmin = std::atof(kws[2].c_str()) * xscale + xofs;
                double zmin = std::atof(kws[3].c_str()) * zscale + zofs;
                double xmax = std::atof(kws[4].c_str()) * xscale + xofs;
                double zmax = std::atof(kws[5].c_str()) * zscale + zofs;
                s << "['rectangle'] = RECTANGLE( " << xmin << ", " << zmin << ", " << xmax << ", " << zmax << " ),\n";
            }
            else
            {
                s << line << '\n';
            }
        });
    }

    BackupFile(targetFilename);
    std::ofstream ofs(targetFilename.toLatin1().data());
    ofs << oss.str();
}



void UpdateMapScenarioFile(QString sourceFilename, QString targetFilename, nfa::scmp::Scmp *scmp)
{
    if (!QFileInfo(sourceFilename).exists())
    {
        return;
    }

    std::ostringstream oss;
    {
        std::ifstream ifs(sourceFilename.toLatin1().data());

        ModifyLines(ifs, oss, [scmp](std::ostream &s, std::string &line, const std::vector<std::string> &kws)
        {
            if (kws.size() == 3 && kws[0] == "size")
            {
                s << "size = { " << scmp->width << ", " << scmp->height << " },\n";
            }
            else
            {
                s << line << '\n';
            }
        });
    }

    BackupFile(targetFilename);
    std::ofstream ofs(targetFilename.toLatin1().data());
    ofs << oss.str();
}


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
    }
    else
    {
        ui.goButton->setEnabled(m_sourceScmp && !getTargetFilename().isEmpty());
    }

    ui.mergePositionFrame->setHidden(!isMergeModeSelected());
    ui.sourceNewWidthComboBox->setHidden(isMergeModeSelected());
    ui.sourceNewHeightComboBox->setHidden(isMergeModeSelected());
    ui.sourceNewWidthSpinBox->setHidden(!isMergeModeSelected());
    ui.sourceNewHeightSpinBox->setHidden(!isMergeModeSelected());
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
    // user might have been fiddling with files in between selecting them and pressing go
    m_sourceScmp = tryLoadScmp(getSourceFilename());
    m_targetScmp = tryLoadScmp(getTargetFilename());

    QFileInfo checkFile(getTargetFilename());
    if (checkFile.exists() && checkFile.isFile())
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "File exists.  Overwrite?", getTargetFilename(), QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }

        BackupFile(getTargetFilename());
    }

    try
    {
        if (isMergeModeSelected() && m_sourceScmp && m_targetScmp)
        {
            double xscale = double(getNewSourceWidth()) / double(m_sourceScmp->width);
            double zscale = double(getNewSourceHeight()) / double(m_sourceScmp->height);
            double xofs = double(getHorzPosition());
            double zofs = double(getVertPosition());
            m_sourceScmp->Resize(getNewSourceWidth(), getNewSourceHeight());
            m_targetScmp->Import(*m_sourceScmp, getHorzPosition(), getVertPosition(), isAdditiveMerge());
            std::ofstream ofs(getTargetFilename().toLatin1().data(), std::ios::binary);
            m_targetScmp->Save(ofs);

            if (getSourceFilename() == getTargetFilename())
            {
                auto targetFilenames = GetMapLuaFileNames(getTargetFilename());
                RescaleMapSaveFile(targetFilenames["save"], targetFilenames["save"], xscale, zscale, xofs, zofs, m_targetScmp.get());
                QMessageBox::information(this,
                    "Rescale/import", "Finished rescaling and importing .scmap and _save.lua", QMessageBox::Ok);
            }
            else
            {
                QMessageBox::information(this,
                    "Rescale/import", "Finished rescaling and importing .scmap.\n"
                    "This tool only edits the .scmap file.  Your next step is:\n"
                    "- use a map editor to place markers from the imported map\n"
                    "  (eg mexes and starting positions)", QMessageBox::Ok);
            }
        }
        else if (!isMergeModeSelected() && m_sourceScmp)
        {
            int newWidthHeight = std::max(getNewSourceWidth(), getNewSourceHeight());
            double xscale = double(newWidthHeight) / double(m_sourceScmp->width);
            double zscale = double(newWidthHeight) / double(m_sourceScmp->height);

            m_sourceScmp->Resize(newWidthHeight, newWidthHeight);
            std::ofstream ofs(getTargetFilename().toLatin1().data(), std::ios::binary);
            m_sourceScmp->Save(ofs);

            auto sourceFilenames = GetMapLuaFileNames(getSourceFilename());
            auto targetFilenames = GetMapLuaFileNames(getTargetFilename());
            RescaleMapSaveFile(sourceFilenames["save"], targetFilenames["save"], xscale, zscale, 0.0, 0.0, m_sourceScmp.get());
            UpdateMapScenarioFile(sourceFilenames["scenario"], targetFilenames["scenario"], m_sourceScmp.get());
            if (sourceFilenames["script"] != targetFilenames["script"] && QFileInfo(sourceFilenames["script"]).exists())
            {
                BackupFile(targetFilenames["script"]);
                QFile::remove(targetFilenames["script"]);
                QFile::copy(sourceFilenames["script"], targetFilenames["script"]);
            }

            QMessageBox::information(this, "Rescale", 
                "Finished rescaling .scmap, _save.lua and _scenario.lua files:\n", QMessageBox::Ok);
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
    if (!ui.sourceNewWidthSpinBox->isHidden())
    {
        return ui.sourceNewWidthSpinBox->value();
    }
    else
    {
        return ui.sourceNewWidthComboBox->currentData().toInt();
    }
}

int ScmpRescaleWindow::getNewSourceHeight()
{
    if (!ui.sourceNewHeightSpinBox->isHidden())
    {
        return ui.sourceNewHeightSpinBox->value();
    }
    else
    {
        return ui.sourceNewHeightComboBox->currentData().toInt();
    }
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