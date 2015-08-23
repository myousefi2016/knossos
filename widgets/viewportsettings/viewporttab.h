#ifndef VIEWPORTTAB_H
#define VIEWPORTTAB_H

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QWidget>

class ViewportTab : public QWidget
{
    friend class EventModel;
    friend class MainWindow;
    friend class ViewportSettingsWidget;
    Q_OBJECT
    QFrame separator;
    QLabel generalHeader{"<strong>General</strong>"};
    QCheckBox showScalebarCheckBox{"Show scalebar"};
    QCheckBox showVPDecorationCheckBox{"Show viewport decorations"};
    QCheckBox drawIntersectionsCrossHairCheckBox{"Draw intersections crosshairs"};
    QCheckBox arbitraryModeCheckBox{"Arbitrary viewport orientation"};
    QVBoxLayout generalLayout;
    // 3D viewport
    QLabel viewport3DHeader{"<strong>3D Viewport</strong>"};
    QCheckBox showXYPlaneCheckBox{"Show XY Plane"};
    QCheckBox showXZPlaneCheckBox{"Show XZ Plane"};
    QCheckBox showYZPlaneCheckBox{"Show YZ Plane"};
    QRadioButton boundariesPixelRadioBtn{"Display dataset boundaries in pixels"};
    QRadioButton boundariesPhysicalRadioBtn{"Display dataset boundaries in µm"};
    QCheckBox rotateAroundActiveNodeCheckBox{"Rotate Around Active Node"};
    QVBoxLayout viewport3DLayout;
    QPushButton resetVPsButton{"Reset viewport positions and sizes"};
    QGridLayout mainLayout;
public:
    explicit ViewportTab(QWidget *parent = 0);

signals:
    void showIntersectionsSignal(const bool value);
    void setVPOrientationSignal(const bool arbitrary);
    void setViewportDecorations(const bool);
    void resetViewportPositions();

public slots:

};

#endif // VIEWPORTTAB_H
