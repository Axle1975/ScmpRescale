#include "ui_scmp_rescale_window.h"

#include <memory>

namespace nfa
{
    namespace scmp
    {
        struct Scmp;
    }
}

class ScmpRescaleWindow : public QMainWindow
{
    Q_OBJECT

public:
    ScmpRescaleWindow(QWidget *parent = 0);

    std::shared_ptr<nfa::scmp::Scmp> getSourceScmp();
    std::shared_ptr<nfa::scmp::Scmp> getTargetScmp();
    QString getSourceFilename();
    QString getTargetFilename();
    int getNewSourceWidth();
    int getNewSourceHeight();
    int getHorzPosition();
    int getVertPosition();
    bool isMergeModeSelected();

    private slots:
    void on_sourceMapButton_clicked();
    void on_targetMapButton_clicked();
    void on_sourceMapLineEdit_textChanged(const QString &);
    void on_targetMapLineEdit_textChanged(const QString &);
    void on_sourceNewWidthSpinBox_valueChanged(int);
    void on_sourceNewHeightSpinBox_valueChanged(int);
    void on_mergeModeRadioButton_toggled(bool);
    void on_sourceHorizontalPositionSpinBox_valueChanged(int);
    void on_sourceHorizontalLeftPositionSlider_valueChanged(int);
    void on_sourceHorizontalRightPositionSlider_valueChanged(int);
    void on_sourceVerticalPositionSpinBox_valueChanged(int);
    void on_sourceVerticalTopPositionSlider_valueChanged(int);
    void on_sourceVerticalBottomPositionSlider_valueChanged(int);
    void on_goButton_clicked();
    void on_exitButton_clicked();

private:
    void updateSourceMapInfo();
    void updateTargetMapInfo();
    void updateSaveOptions();
    void updatePositionSliders();
    std::shared_ptr<nfa::scmp::Scmp> tryLoadScmp(const QString &fn);

    Ui::ScmpRescaleWindow ui;

    std::shared_ptr<nfa::scmp::Scmp> m_sourceScmp;
    std::shared_ptr<nfa::scmp::Scmp> m_targetScmp;
};
