#include "robomongo/core/settings/SslSettings.h"

#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    SslSettings::SslSettings() :
            _enabled(false) { }

    SslSettings *SslSettings::clone() const {
        SslSettings *cloned = new SslSettings(*this);
        return cloned;
    }

    QVariant SslSettings::toVariant() const {
        QVariantMap map;
        map.insert("enabled", enabled());   // todo: rename ssl_enabled
        map.insert("caFile", QtUtils::toQString(caFile()));
        map.insert("pemKeyFile", QtUtils::toQString(pemKeyFile()));
        map.insert("pemPassPhrase", QtUtils::toQString(pemPassPhrase()));
        map.insert("crlFile", QtUtils::toQString(crlFile()));
        map.insert("allowInvalidHostnames", allowInvalidHostnames());
        map.insert("allowInvalidCertificates", allowInvalidCertificates());
        map.insert("pemKeyEncrypted", pemKeyEncrypted());

        return map;
    }

    void SslSettings::fromVariant(const QVariantMap &map) {
        enableSSL(map.value("enabled").toBool());
        setCaFile(QtUtils::toStdString(map.value("caFile").toString()));
        setPemKeyFile(QtUtils::toStdString(map.value("pemKeyFile").toString()));
        setPemPassPhrase(QtUtils::toStdString(map.value("pemPassPhrase").toString()));
        setCrlFile(QtUtils::toStdString(map.value("crlFile").toString()));
        setAllowInvalidHostnames(map.value("allowInvalidHostnames").toBool());
        setAllowInvalidCertificates(map.value("allowInvalidCertificates").toBool());
        setPemKeyEncrypted(map.value("pemKeyEncrypted").toBool());
    }
}
