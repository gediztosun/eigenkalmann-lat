#include "scatterdatamodifier.h"
#include <Eigen/Dense>
#include <QtCore/qmath.h>
#include <QtCore/qrandom.h>
#include <QtDataVisualization/q3dcamera.h>
#include <QtDataVisualization/q3dscene.h>
#include <QtDataVisualization/q3dtheme.h>
#include <QtDataVisualization/qscatter3dseries.h>
#include <QtDataVisualization/qscatterdataproxy.h>
#include <QtDataVisualization/qvalue3daxis.h>
#include <QtWidgets/QComboBox>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <fstream>
#include <iostream>


using namespace QtDataVisualization;


// #define RANDOM_SCATTER // Uncomment this to switch to random scatter

const int numberOfItems = 3600;
const float curveDivider = 3.0f;
const int lowerNumberOfItems = 900;
const float lowerCurveDivider = 0.75f;

ScatterDataModifier::
ScatterDataModifier(Q3DScatter* scatter, std::vector<std::vector<double>> &pos, std::vector<std::vector<double>> &posEs) :
        m_graph(scatter), position(pos), m_positionEstimates(posEs), m_fontSize(40.0f), m_style(QAbstract3DSeries::MeshSphere), m_smooth(true),
        m_itemCount(lowerNumberOfItems), m_curveDivider(lowerCurveDivider) {
    //! [0]
    m_graph->activeTheme()->setType(Q3DTheme::ThemeEbony);
    QFont font = m_graph->activeTheme()->font();
    font.setPointSize(m_fontSize);
    m_graph->activeTheme()->setFont(font);
    m_graph->setShadowQuality(QAbstract3DGraph::ShadowQualitySoftLow);
    m_graph->scene()->activeCamera()->setCameraPreset(Q3DCamera::CameraPresetFront);
    //! [0]

    //! [2]
    QScatterDataProxy* proxy = new QScatterDataProxy;
    QScatter3DSeries* series = new QScatter3DSeries(proxy);
    series->setItemLabelFormat(QStringLiteral("@xTitle: @xLabel @yTitle: @yLabel @zTitle: @zLabel"));
    series->setMeshSmooth(m_smooth);
    m_graph->addSeries(series);
    //! [2]

    QScatterDataProxy* proxyEs = new QScatterDataProxy;
    QScatter3DSeries* seriesEs = new QScatter3DSeries(proxyEs);
    seriesEs->setItemLabelFormat(QStringLiteral("@xTitle: @xLabel @yTitle: @yLabel @zTitle: @zLabel"));
    seriesEs->setMeshSmooth(m_smooth);
    m_graph->addSeries(seriesEs);

    //! [3]
    addData(400);
    //! [3]
}

ScatterDataModifier::~
ScatterDataModifier() {
    delete m_graph;
}

void
ScatterDataModifier::addData(int skipValue) {
    // Configure the axes according to the data
    //! [4]
    m_graph->axisX()->setTitle("X");

    m_graph->axisY()->setTitle("Y");
    m_graph->axisY()->setRange(-1, 1);

    m_graph->axisZ()->setTitle("Z");
    //! [4]

    int skipRate = 400 + 1 - skipValue;
    skipRate = qMax(1, skipRate);

    /*
    QScatterDataArray* dataArray = new QScatterDataArray;
    dataArray->resize(position.size());
    QScatterDataItem* ptrToDataArray = &dataArray->first();
    //! [5]

    for (const auto& p: position) {
        ptrToDataArray->setPosition(
                QVector3D(static_cast<float>(p[1]), static_cast<float>(p[2]), static_cast<float>(p[0])));
        ptrToDataArray++;
    }

    //! [7]
    m_graph->seriesList().at(0)->dataProxy()->resetArray(dataArray);
    //! [7]
    */

    QScatterDataArray* dataArray = new QScatterDataArray;
    int finalDataCountReal = std::min(position.size(), (position.size() + skipRate - 1) / skipRate);
    dataArray->resize(finalDataCountReal);
    // std::cout << m_positionEstimates.size() << std::endl;
    QScatterDataItem* ptrToDataArray = &dataArray->first();

    for (int i = 0; i < position.size(); i += skipRate) {

        const auto &p = position[i];
        ptrToDataArray->setPosition(QVector3D(static_cast<float>(p[1]), static_cast<float>(p[2]), static_cast<float>(p[0])));
        ptrToDataArray++;
    }
    m_graph->seriesList().at(0)->dataProxy()->resetArray(dataArray);

    QScatterDataArray* dataArrayEs = new QScatterDataArray;
    int finalDataCount = std::min(m_positionEstimates.size(), (m_positionEstimates.size() + skipRate - 1) / skipRate);
    dataArrayEs->resize(finalDataCount);
    // std::cout << m_positionEstimates.size() << std::endl;
    QScatterDataItem* ptrToDataArrayEs = &dataArrayEs->first();

    for (int i = 0; i < m_positionEstimates.size(); i += skipRate) {

        const auto &p = m_positionEstimates[i];
        ptrToDataArrayEs->setPosition(QVector3D(static_cast<float>(p[1]), static_cast<float>(p[2]), static_cast<float>(p[0])));
        ptrToDataArrayEs++;
    }
    m_graph->seriesList().at(1)->dataProxy()->resetArray(dataArrayEs);
}

//! [8]
void
ScatterDataModifier::changeStyle(int style) {
    QComboBox* comboBox = qobject_cast<QComboBox*>(sender());
    if (comboBox) {
        m_style = QAbstract3DSeries::Mesh(comboBox->itemData(style).toInt());
        if (m_graph->seriesList().size()) {
            m_graph->seriesList().at(0)->setMesh(m_style);
            m_graph->seriesList().at(1)->setMesh(m_style);
        }
    }
}

void
ScatterDataModifier::setSmoothDots(int smooth) {
    m_smooth = bool(smooth);
    QScatter3DSeries* series = m_graph->seriesList().at(0);
    series->setMeshSmooth(m_smooth);
    QScatter3DSeries* series2 = m_graph->seriesList().at(1);
    series2->setMeshSmooth(m_smooth);
}

void
ScatterDataModifier::changeTheme(int theme) {
    Q3DTheme* currentTheme = m_graph->activeTheme();
    currentTheme->setType(Q3DTheme::Theme(theme));
    emit backgroundEnabledChanged(currentTheme->isBackgroundEnabled());
    emit gridEnabledChanged(currentTheme->isGridEnabled());
    emit fontChanged(currentTheme->font());
}

void
ScatterDataModifier::changePresetCamera() {
    static int preset = Q3DCamera::CameraPresetFrontLow;

    m_graph->scene()->activeCamera()->setCameraPreset((Q3DCamera::CameraPreset) preset);

    if (++preset > Q3DCamera::CameraPresetDirectlyBelow)
        preset = Q3DCamera::CameraPresetFrontLow;
}

void
ScatterDataModifier::changeLabelStyle() {
    m_graph->activeTheme()->setLabelBackgroundEnabled(!m_graph->activeTheme()->isLabelBackgroundEnabled());
}

void
ScatterDataModifier::changeFont(const QFont& font) {
    QFont newFont = font;
    newFont.setPointSizeF(m_fontSize);
    m_graph->activeTheme()->setFont(newFont);
}

void
ScatterDataModifier::shadowQualityUpdatedByVisual(QAbstract3DGraph::ShadowQuality sq) {
    int quality = int(sq);
    emit shadowQualityChanged(quality); // connected to a checkbox in main.cpp
}

void
ScatterDataModifier::changeShadowQuality(int quality) {
    QAbstract3DGraph::ShadowQuality sq = QAbstract3DGraph::ShadowQuality(quality);
    m_graph->setShadowQuality(sq);
}

void
ScatterDataModifier::setBackgroundEnabled(int enabled) {
    m_graph->activeTheme()->setBackgroundEnabled((bool) enabled);
}

void
ScatterDataModifier::setGridEnabled(int enabled) {
    m_graph->activeTheme()->setGridEnabled((bool) enabled);
}

//! [8]

void
ScatterDataModifier::toggleItemCount() {

    if (m_itemCount == numberOfItems) {
        m_itemCount = lowerNumberOfItems;
        m_curveDivider = lowerCurveDivider;
    } else {
        m_itemCount = numberOfItems;
        m_curveDivider = curveDivider;
    }
    m_graph->seriesList().at(0)->dataProxy()->resetArray(0);
}

QVector3D
ScatterDataModifier::randVector() {
    return QVector3D((float) (QRandomGenerator::global()->bounded(100)) / 2.0f -
                     (float) (QRandomGenerator::global()->bounded(100)) / 2.0f,
                     (float) (QRandomGenerator::global()->bounded(100)) / 100.0f -
                     (float) (QRandomGenerator::global()->bounded(100)) / 100.0f,
                     (float) (QRandomGenerator::global()->bounded(100)) / 2.0f -
                     (float) (QRandomGenerator::global()->bounded(100)) / 2.0f);
}