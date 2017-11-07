/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "dataset.h"

#include "network.h"
#include "segmentation/segmentation.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>
#include <QUrlQuery>

Dataset::list_t Dataset::datasets;

QString Dataset::compressionString() const {
    switch (type) {
    case Dataset::CubeType::RAW_UNCOMPRESSED: return "8 bit gray";
    case Dataset::CubeType::RAW_JPG: return "jpg";
    case Dataset::CubeType::RAW_J2K: return "j2k";
    case Dataset::CubeType::RAW_JP2_6: return "jp2";
    case Dataset::CubeType::SEGMENTATION_UNCOMPRESSED_16: return "16 bit id";
    case Dataset::CubeType::SEGMENTATION_UNCOMPRESSED_64: return "64 bit id";
    case Dataset::CubeType::SEGMENTATION_SZ_ZIP: return "sz.zip";
    case Dataset::CubeType::SNAPPY: return "snappy";
    }
    throw std::runtime_error(QObject::tr("no compressions string for %1").arg(static_cast<int>(type)).toUtf8()); ;
}

bool Dataset::isHeidelbrain(const QUrl & url) {
    return !isNeuroDataStore(url) && !isWebKnossos(url);
}

bool Dataset::isNeuroDataStore(const QUrl & url) {
    return url.path().contains("/nd/sd/") || url.path().contains("/ocp/ca/");
}

bool Dataset::isWebKnossos(const QUrl & url) {
    return url.host().contains("webknossos");
}

Dataset::list_t Dataset::parse(const QUrl & url, const QString & data) {
    if (Dataset::isWebKnossos(url)) {
        return Dataset::parseWebKnossosJson(url, data);
    } else if (Dataset::isNeuroDataStore(url)) {
        return Dataset::parseNeuroDataStoreJson(url, data);
    } else {
        return Dataset::fromLegacyConf(url, data);
    }
}

Dataset::list_t Dataset::parseGoogleJson(const QString & json_raw) {
    Dataset info;
    info.api = API::GoogleBrainmaps;
    const auto jmap = QJsonDocument::fromJson(json_raw.toUtf8()).object();

    const auto boundary_json = jmap["geometry"].toArray()[0].toObject()["volumeSize"].toObject();
    info.boundary = {
        boundary_json["x"].toString().toInt(),
        boundary_json["y"].toString().toInt(),
        boundary_json["z"].toString().toInt(),
    };

    const auto scale_json = jmap["geometry"].toArray()[0].toObject()["pixelSize"].toObject();
    info.scale = {
        static_cast<float>(scale_json["x"].toDouble()),
        static_cast<float>(scale_json["y"].toDouble()),
        static_cast<float>(scale_json["z"].toDouble()),
    };

    info.lowestAvailableMag = 1;
    info.magnification = info.lowestAvailableMag;
    info.highestAvailableMag = std::pow(2,(jmap["geometry"].toArray().size()-1)); //highest google mag
    info.type = CubeType::RAW_JPG;

    return {info};
}

Dataset::list_t Dataset::parseNeuroDataStoreJson(const QUrl & infoUrl, const QString & json_raw) {
    Dataset info;
    info.api = API::OpenConnectome;
    info.url = infoUrl;
    info.url.setPath(info.url.path().replace(QRegularExpression{"\\/info\\/?"}, "/image/jpeg/"));
    const auto dataset = QJsonDocument::fromJson(json_raw.toUtf8()).object()["dataset"].toObject();
    const auto imagesize0 = dataset["imagesize"].toObject()["0"].toArray();
    info.boundary = {
        imagesize0[0].toInt(),
        imagesize0[1].toInt(),
        imagesize0[2].toInt(),
    };
    const auto voxelres0 = dataset["voxelres"].toObject()["0"].toArray();
    info.scale = {
        static_cast<float>(voxelres0[0].toDouble()),
        static_cast<float>(voxelres0[1].toDouble()),
        static_cast<float>(voxelres0[2].toDouble()),
    };
    const auto mags = dataset["resolutions"].toArray();

    info.lowestAvailableMag = std::pow(2, mags[0].toInt());
    info.magnification = info.lowestAvailableMag;
    info.highestAvailableMag = std::pow(2, mags[mags.size()-1].toInt());
    info.type = CubeType::RAW_JPG;

    return {info};
}

Dataset::list_t Dataset::parseWebKnossosJson(const QUrl & infoUrl, const QString & json_raw) {
    Dataset info;
    info.api = API::WebKnossos;

    const auto jmap = QJsonDocument::fromJson(json_raw.toUtf8()).object();

    info.url = jmap["dataStore"].toObject()["url"].toString() + "/data/datasets/" + jmap["name"].toString();

    decltype(Dataset::datasets) layers;
    for (auto && layer : jmap["dataSource"].toObject()["dataLayers"].toArray()) {
        const auto layerString = layer.toObject()["name"].toString();
        const auto category = layer.toObject()["category"].toString();
        const auto download = Network::singleton().refresh(QString("https://demo.webknossos.org/dataToken/generate?dataSetName=%1&dataLayerName=%2").arg(infoUrl.path().split("/").back()).arg(layerString));
        if (download.first) {
            info.token = QJsonDocument::fromJson(download.second.toUtf8()).object()["token"].toString();
        }
        if (category == "color") {
            info.type = CubeType::RAW_UNCOMPRESSED;
        } else {// "segmentation"
            info.type = CubeType::SEGMENTATION_UNCOMPRESSED_16;
        }
        const auto boundary_json = layer.toObject()["boundingBox"].toObject();
        info.boundary = {
            boundary_json["width"].toInt(),
            boundary_json["height"].toInt(),
            boundary_json["depth"].toInt(),
        };

        const auto scale_json = jmap["dataSource"].toObject()["scale"].toArray();
        info.scale = {
            static_cast<float>(scale_json[0].toDouble()),
            static_cast<float>(scale_json[1].toDouble()),
            static_cast<float>(scale_json[2].toDouble()),
        };

        auto mags = layer.toObject()["resolutions"].toArray().toVariantList();
        std::sort(std::begin(mags), std::end(mags), [](auto lhs, auto rhs){
            return lhs.toInt() < rhs.toInt();
        });

        info.lowestAvailableMag = mags[0].toInt();
        info.magnification = info.lowestAvailableMag;
        info.highestAvailableMag = mags[mags.size()-1].toInt();

        layers.push_back(info);
        layers.back().url.setPath(info.url.path() + "/layers/" + layerString + "/data");
        layers.back().url.setQuery(info.url.query().append("token=" + info.token));
    }

    return layers;
}

Dataset::list_t Dataset::fromLegacyConf(const QUrl & configUrl, QString config) {
    Dataset info;
    info.api = API::Heidelbrain;

    QTextStream stream(&config);
    QString line;
    while (!(line = stream.readLine()).isNull()) {
        const QStringList tokenList = line.split(QRegularExpression("[ ;]"));
        QString token = tokenList.front();

        if (token == "experiment") {
            token = tokenList.at(2);
            info.experimentname = token.remove('\"');
        } else if (token == "scale") {
            token = tokenList.at(1);
            if(token == "x") {
                info.scale.x = tokenList.at(2).toFloat();
            } else if(token == "y") {
                info.scale.y = tokenList.at(2).toFloat();
            } else if(token == "z") {
                info.scale.z = tokenList.at(2).toFloat();
            }
        } else if (token == "boundary") {
            token = tokenList.at(1);
            if(token == "x") {
                info.boundary.x = tokenList.at(2).toFloat();
            } else if (token == "y") {
                info.boundary.y = tokenList.at(2).toFloat();
            } else if (token == "z") {
                info.boundary.z = tokenList.at(2).toFloat();
            }
        } else if (token == "magnification") {
            info.magnification = tokenList.at(1).toInt();
        } else if (token == "cube_edge_length") {
            info.cubeEdgeLength = tokenList.at(1).toInt();
        } else if (token == "ftp_mode") {
            info.remote = true;
            auto maybeUrl = tokenList.at(1);
            if (QUrl{maybeUrl}.scheme().isEmpty()) {
                maybeUrl.prepend("http://");
            }
            info.url = maybeUrl;
            if (tokenList.size() >= 3 && !tokenList.at(2).isEmpty()) {
                info.url.setPath(tokenList.at(2));
            }
            if (tokenList.size() >= 5) {
                info.url.setUserName(tokenList.at(3));
                info.url.setPassword(tokenList.at(4));
            }
            // discarding ftpFileTimeout parameter
        } else if (token == "compression_ratio") {
            const auto compressionRatio = tokenList.at(1).toInt();
            info.type = compressionRatio == 0 ? Dataset::CubeType::RAW_UNCOMPRESSED
                      : compressionRatio == 1000 ? Dataset::CubeType::RAW_JPG
                      : compressionRatio == 6 ? Dataset::CubeType::RAW_JP2_6
                      : Dataset::CubeType::RAW_J2K;
        } else {
            qDebug() << "Skipping unknown parameter" << token;
        }
    }

    if (info.url.isEmpty()) {
        //find root of dataset if conf was inside mag folder
        auto configDir = QFileInfo(configUrl.toLocalFile()).absoluteDir();
        if (QRegularExpression("^mag[0-9]+$").match(configDir.dirName()).hasMatch()) {
            configDir.cdUp();//support
        }
        info.url = QUrl::fromLocalFile(configDir.absolutePath());
    }

    //transform boundary and scale of higher mag only datasets
    info.boundary = info.boundary * info.magnification;
    info.scale = info.scale / static_cast<float>(info.magnification);
    info.lowestAvailableMag = info.highestAvailableMag = info.magnification;

    return {info};
}

void Dataset::checkMagnifications() {
    //iterate over all possible mags and test their availability
    std::tie(lowestAvailableMag, highestAvailableMag) = Network::singleton().checkOnlineMags(url);
    qDebug() << QObject::tr("Lowest Mag: %1, Highest Mag: %2").arg(lowestAvailableMag).arg(highestAvailableMag).toUtf8().constData();
}

Dataset Dataset::createCorrespondingOverlayLayer() {
    Dataset info = *this;
    info.type = api == API::Heidelbrain ? CubeType::SEGMENTATION_SZ_ZIP : CubeType::SEGMENTATION_UNCOMPRESSED_64;
    return info;
}

QUrl Dataset::knossosCubeUrl(const Coordinate coord) const {
    const auto cubeCoord = coord.cube(cubeEdgeLength, magnification);
    auto pos = QString("/mag%1/x%2/y%3/z%4/")
            .arg(magnification)
            .arg(cubeCoord.x, 4, 10, QChar('0'))
            .arg(cubeCoord.y, 4, 10, QChar('0'))
            .arg(cubeCoord.z, 4, 10, QChar('0'));
    auto filename = QString(("%1_mag%2_x%3_y%4_z%5%6"))//2012-03-07_AreaX14_mag1_x0000_y0000_z0000.j2k
            .arg(experimentname.section(QString("_mag"), 0, 0))
            .arg(magnification)
            .arg(cubeCoord.x, 4, 10, QChar('0'))
            .arg(cubeCoord.y, 4, 10, QChar('0'))
            .arg(cubeCoord.z, 4, 10, QChar('0'));

    if (type == Dataset::CubeType::RAW_UNCOMPRESSED) {
        filename = filename.arg(".raw");
    } else if (type == Dataset::CubeType::RAW_JPG) {
        filename = filename.arg(".jpg");
    } else if (type == Dataset::CubeType::RAW_J2K) {
        filename = filename.arg(".j2k");
    } else if (type == Dataset::CubeType::RAW_JP2_6) {
        filename = filename.arg(".6.jp2");
    } else if (type == Dataset::CubeType::SEGMENTATION_SZ_ZIP) {
        filename = filename.arg(".seg.sz.zip");
    } else if (type == Dataset::CubeType::SEGMENTATION_UNCOMPRESSED_64) {
        filename = filename.arg(".seg");
    }

    auto base = url;
    base.setPath(url.path() + pos + filename);

    return base;
}

QUrl Dataset::googleCubeUrl(const Coordinate coord) const {
    auto path = url.path() + "/binary/subvolume";

    if (type == Dataset::CubeType::RAW_UNCOMPRESSED) {
        path += "/format=raw";
    } else if (type == Dataset::CubeType::RAW_JPG) {
        path += "/format=singleimage";
    }
    path += "/scale=" + QString::number(int_log(magnification));// >= 0
    path += "/size=" + QString("%1,%1,%1").arg(cubeEdgeLength);// <= 128³
    path += "/corner=" + QString("%1,%2,%3").arg(coord.x).arg(coord.y).arg(coord.z);

    auto query = QUrlQuery(url);
    query.addQueryItem("alt", "media");

    auto base = url;
    base.setPath(path);
    base.setQuery(query);
    //<volume_id>/binary/subvolume/corner=5376,5504,2944/size=64,64,64/scale=0/format=singleimage?access_token=<oauth2_token>
    return base;
}

QUrl Dataset::openConnectomeCubeUrl(Coordinate coord) const {
    auto path = url.path();

    path += (!path.endsWith('/') ? "/" : "") + QString::number(int_log(magnification));// >= 0
    coord.x /= magnification;
    coord.y /= magnification;
    coord.z += 1;//offset
    path += "/" + QString("%1,%2").arg(coord.x).arg(coord.x + cubeEdgeLength);
    path += "/" + QString("%1,%2").arg(coord.y).arg(coord.y + cubeEdgeLength);
    path += "/" + QString("%1,%2").arg(coord.z).arg(coord.z + cubeEdgeLength);

    auto base = url;
    base.setPath(path + "/");
    //(string: server_name)/ocp/ca/(string: token_name)/(string: channel_name)/jpeg/(int: resolution)/(int: min_x),(int: max_x)/(int: min_y),(int: max_y)/(int: min_z),(int: max_z)/
    //(string: server_name)/nd/sd/(string: token_name)/(string: channel_name)/jpeg/(int: resolution)/(int: min_x),(int: max_x)/(int: min_y),(int: max_y)/(int: min_z),(int: max_z)/
    return base;
}

QUrl Dataset::apiSwitch(const Coordinate globalCoord) const {
    switch (api) {
    case API::GoogleBrainmaps:
        return googleCubeUrl(globalCoord);
    case API::Heidelbrain:
        return knossosCubeUrl(globalCoord);
    case API::OpenConnectome:
        return openConnectomeCubeUrl(globalCoord);
    case API::WebKnossos:
        return url;
    }
    throw std::runtime_error("unknown value for Dataset::API");
}

bool Dataset::isOverlay() const {
    return type == CubeType::SEGMENTATION_UNCOMPRESSED_16
            || type == CubeType::SEGMENTATION_UNCOMPRESSED_64
            || type == CubeType::SEGMENTATION_SZ_ZIP
            || type == CubeType::SNAPPY;
}
