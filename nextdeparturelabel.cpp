#include "nextdeparturelabel.h"

#include <QMap>
#include <QVariant>
#include <QXmlStreamReader>

#include "departureinfo.h"


const QUrl NextDepartureLabel::baseUrl = QUrl("http://api.vasttrafik.se/bin/rest.exe", QUrl::StrictMode);
const QString NextDepartureLabel::key = "ac6fb7fc-c5c5-482a-bc8d-35c439018468";
const double NextDepartureLabel::chalmersLatitude = 57.689955;
const double NextDepartureLabel::chalmersLongitude =  11.972930;


NextDepartureLabel::NextDepartureLabel(QWidget *parent) :
    QLabel(parent),
    stationId(0),
    stationHttpGetId(0),
    departureHttpGetId(0)
{
    connect(&http, SIGNAL(requestFinished(int,bool)),
            this, SLOT(httpRequestFinished(int,bool)));
    connect(&updateTimer, SIGNAL(timeout()),
            this, SLOT(updateDepartures()));
    updateDepartures();
}

int NextDepartureLabel::getUpdateInterval() const
{
    return updateTimer.interval();
}

void NextDepartureLabel::setUpdateInterval(int seconds)
{
    updateTimer.stop();
    updateTimer.setInterval(seconds * 1000);
    if (seconds != 0) {
        updateTimer.start();
    }
}

void NextDepartureLabel::updateDepartures()
{
    getStationId();
}

void NextDepartureLabel::httpRequestFinished(int id, bool error)
{
    if (error) {
        clear();
        setText(http.errorString());
    } else if (id == stationHttpGetId) {
        QXmlStreamReader xml;
        xml.addData(http.readAll());

        if (xml.readNextStartElement() && xml.name() == "LocationList") {
            if (xml.readNextStartElement() && xml.name() == "StopLocation") {
                if (xml.attributes().hasAttribute("id")) {
                    stationId = xml.attributes().value("id").toString().toULongLong();
                }
            }
        }

        if (xml.hasError()) {
            setText(xml.errorString());
        } else {
            getDepartures();
        }
    } else if (id == departureHttpGetId) {
        QXmlStreamReader xml;
        xml.addData(http.readAll());

        if (xml.readNextStartElement() && xml.name() == "DepartureBoard") {
            while (xml.readNextStartElement()) {
                if (xml.name() == "Departure") {
                    departures.append(DepartureInfo(xml));
                }
                xml.skipCurrentElement();
            }
        }

        if (xml.hasError()) {
            setText(xml.errorString());
        } else if (!departures.isEmpty()) {
            const DepartureInfo &info = departures.at(0);
            QString text = info.time.toString("hh:mm") + " - " + info.name + " towards " + info.direction;
            for (int i = 1; i < departures.size() && i < 2; ++i) {
                const DepartureInfo &info = departures.at(i);
                text += ", " + info.time.toString("hh:mm") + " - " + info.name + " towards " + info.direction;
            }
            setText(text);
        }
    }
}

void NextDepartureLabel::getStationId()
{
    QUrl url(baseUrl);
    url.setPath(baseUrl.path() + "/location.nearbystops");
    url.setQueryDelimiters('=', '&');

    QMap<QString, QVariant> params;
    params["originCoordLat"] = chalmersLatitude;
    params["originCoordLong"] = chalmersLongitude;
    params["maxNo"] = 1;

    QMap<QString, QVariant>::const_iterator it = params.constBegin();
    while (it != params.constEnd()) {
        url.addQueryItem(it.key(), it.value().toString());
        ++it;
    }

    url.addQueryItem("authKey", key);

    http.setHost(baseUrl.host(), baseUrl.port(80));
    stationHttpGetId = http.get(url.toString());
}

void NextDepartureLabel::getDepartures()
{
    QUrl url(baseUrl);
    url.setPath(baseUrl.path() + "/departureBoard");
    url.setQueryDelimiters('=', '&');

    QMap<QString, QVariant> params;
    params["id"] = stationId;
    params["maxDeparturesPerLine"] = 1;

    QMap<QString, QVariant>::const_iterator it = params.constBegin();
    while (it != params.constEnd()) {
        url.addQueryItem(it.key(), it.value().toString());
        ++it;
    }

    url.addQueryItem("authKey", key);

    http.setHost(baseUrl.host(), baseUrl.port(80));
    departureHttpGetId = http.get(url.toString());
}
