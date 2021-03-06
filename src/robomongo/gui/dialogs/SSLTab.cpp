#include "robomongo/gui/dialogs/SSLTab.h"

#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QComboBox>
#include <QRadioButton>
#include <QFileInfo>
#include <QMessageBox>
#include <QEvent>

#include <mongo/util/net/ssl_options.h>
#include <mongo/util/net/ssl_manager.h>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/utils/ComboBoxUtils.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/core/settings/SslSettings.h"
#include "robomongo/gui/GuiRegistry.h"

namespace 
{
    // Helper function: Check file existence, return true if file exists, else otherwise
    bool fileExists(const QString &path) 
    {
        QFileInfo fileInfo(path);
        return (fileInfo.exists() && fileInfo.isFile());
    }

    // Helper hint constant strings
    QString const CA_FILE_HINT                      = "( mongo --sslCAFile )";
    QString const CLIENT_CERT_KEY_HINT              = "( mongo --sslPEMKeyFile )";
    QString const CLIENT_CERT_KEY_PASS_HINT         = "( mongo --sslPEMKeyPassword )";
    QString const ALLOW_INVALID_HOSTNAME_HINT       = "( mongo --sslAllowInvalidHostnames )";
    QString const ALLOW_INVALID_CERTIFICATES_HINT   = "( mongo --sslAllowInvalidCertificates )";
    QString const CRL_FILE_HINT                     = "( mongo --sslCRLFile )";
}

namespace Robomongo
{
    SSLTab::SSLTab(ConnectionSettings *settings) 
        : _settings(settings)
    {
        const SslSettings* const sslSettings = _settings->sslSettings();
        int const WIN_BUTTON_HEIGHT = 23;

        // Use SSL section
        _useSslCheckBox = new QCheckBox("Use SSL protocol");
        _useSslCheckBox->setStyleSheet("margin-bottom: 7px");
        _useSslCheckBox->setChecked(sslSettings->enabled());
        VERIFY(connect(_useSslCheckBox, SIGNAL(stateChanged(int)), this, SLOT(useSslCheckBoxStateChange(int))));

        // Auth Method section
        _authMethodLabel = new QLabel("Authentication Method:        ");
        _authMethodLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        _authMethodComboBox = new QComboBox;
        _authMethodComboBox->addItem("Self-signed Certificate");
        _authMethodComboBox->addItem("CA-signed Certificate");
        _selfSignedInfoStr = new QLabel("In general, avoid using self-signed certificates unless the network is trusted.");
        _selfSignedInfoStr->setWordWrap(true);
        _caFileLabel = new QLabel("CA-signed Certificate: ");
        _caFilePathLineEdit = new QLineEdit;
        _caFileBrowseButton = new QPushButton("...");
        _caFileBrowseButton->setMaximumWidth(50);
        VERIFY(connect(_caFileBrowseButton, SIGNAL(clicked()), this, SLOT(on_caFileBrowseButton_clicked())));
        VERIFY(connect(_authMethodComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(on_authModeComboBox_change(int))));

        // PEM file section
        _usePemFileCheckBox = new QCheckBox;
        _usePemFileCheckBoxLabel = new QLabel("Use PEM Certificate/Key: ");
        _usePemFileCheckBoxLabel->installEventFilter(this);
        _usePemFileCheckBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        _usePemFileCheckBoxLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        _pemFileInfoStr = 
            new QLabel("Enable this option to connect to a MongoDB that requires CA-signed client certificates/key file.");
        _pemFileInfoStr->setWordWrap(true);
        _pemFileLabel = new QLabel("PEM Certificate/Key: ");
        _pemFileLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        _pemFilePathLineEdit = new QLineEdit;
        _pemFileBrowseButton = new QPushButton("...");
        _pemFileBrowseButton->setMaximumWidth(50);
        VERIFY(connect(_usePemFileCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_usePemFileCheckBox_toggle(bool))));
        VERIFY(connect(_pemFileBrowseButton, SIGNAL(clicked()), this, SLOT(on_pemKeyFileBrowseButton_clicked())));
        // PEM Passphrase section
        _pemPassLabel = new QLabel("Passphrase: ");    
        _pemPassLineEdit = new QLineEdit;
        _pemPassShowButton = new QPushButton;
        togglePassphraseShowMode();
        // Fix for MAC OSX: PEM pass show button was created bigger, making it same size as other pushbuttons
        _pemPassShowButton->setMaximumWidth(_pemFileBrowseButton->width());
        VERIFY(connect(_pemPassShowButton, SIGNAL(clicked()), this, SLOT(togglePassphraseShowMode())));
        _usePemPassphraseCheckBox = new QCheckBox("PEM key is encrypted with passphrase");
        _usePemPassphraseCheckBox->setChecked(sslSettings->pemKeyEncrypted());
        VERIFY(connect(_usePemPassphraseCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_usePemPassphraseCheckBox_toggle(bool))));

        // Advanced options
        _useAdvancedOptionsCheckBox = new QCheckBox;
        _useAdvancedOptionsCheckBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        _useAdvancedOptionsLabel = new QLabel("Use Advanced Options");
        _useAdvancedOptionsLabel->installEventFilter(this);
        _useAdvancedOptionsLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        VERIFY(connect(_useAdvancedOptionsCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_useAdvancedOptionsCheckBox_toggle(bool))));
        _crlFileLabel = new QLabel("CRL (Revocation List): ");
        _crlFilePathLineEdit = new QLineEdit;
        _crlFileBrowseButton = new QPushButton("...");
        _crlFileBrowseButton->setMaximumWidth(50);
        VERIFY(connect(_crlFileBrowseButton, SIGNAL(clicked()), this, SLOT(on_crlFileBrowseButton_clicked())));
        _allowInvalidHostnamesLabel = new QLabel("Allow Invalid Hostnames: ");
        _allowInvalidHostnamesComboBox = new QComboBox;
        _allowInvalidHostnamesComboBox->addItem("No");
        _allowInvalidHostnamesComboBox->addItem("Yes");
        _allowInvalidHostnamesComboBox->setCurrentIndex(sslSettings->allowInvalidHostnames());

        // Layouts
        // CA section
        QGridLayout* gridLayout = new QGridLayout;
        gridLayout->addWidget(_authMethodLabel,                 0 ,0, 1, 2);
        gridLayout->addWidget(_authMethodComboBox,              0 ,2, 1, 2);
        gridLayout->addWidget(_selfSignedInfoStr,               1, 2, 2, 2);
        gridLayout->addWidget(_caFileLabel,                     2, 1);
        gridLayout->addWidget(_caFilePathLineEdit,              2, 2);
        gridLayout->addWidget(_caFileBrowseButton,              2, 3);        
        gridLayout->addWidget(new QLabel(""),                   3, 0);        
        // PEM File Section
        gridLayout->addWidget(_usePemFileCheckBox,              4, 0, Qt::AlignTop);
        gridLayout->addWidget(_usePemFileCheckBoxLabel,         4, 1, Qt::AlignTop);
        gridLayout->addWidget(_pemFileInfoStr,                  4, 2, 1, 2);
        gridLayout->addWidget(_pemFileLabel,                    5, 1);
        gridLayout->addWidget(_pemFilePathLineEdit,             5, 2);
        gridLayout->addWidget(_pemFileBrowseButton,             5, 3);
        gridLayout->addWidget(_pemPassLabel,                    6, 1);
        gridLayout->addWidget(_pemPassLineEdit,                 6, 2);
        gridLayout->addWidget(_pemPassShowButton,               6, 3);
        gridLayout->addWidget(_usePemPassphraseCheckBox,        7, 2, 1, 2);
        gridLayout->addWidget(new QLabel(""),                   8, 1);        
        // Advanced section
        gridLayout->addWidget(_useAdvancedOptionsCheckBox,       9, 0, Qt::AlignTop);
        gridLayout->addWidget(_useAdvancedOptionsLabel,          9, 1, Qt::AlignTop);
        gridLayout->addWidget(_crlFileLabel,                     10, 1);
        gridLayout->addWidget(_crlFilePathLineEdit,              10, 2);
        gridLayout->addWidget(_crlFileBrowseButton,              10, 3);
        gridLayout->addWidget(_allowInvalidHostnamesLabel,       11, 1);
        gridLayout->addWidget(_allowInvalidHostnamesComboBox,    11, 2, Qt::AlignLeft);
        gridLayout->addWidget(new QLabel(""),                    12, 0);

        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->setAlignment(Qt::AlignTop);
        mainLayout->addWidget(_useSslCheckBox);
        mainLayout->addWidget(new QLabel(""));
        mainLayout->addLayout(gridLayout);
        setLayout(mainLayout);

        // Update widget states according to SSL settings
        useSslCheckBoxStateChange(_useSslCheckBox->checkState());
        _caFilePathLineEdit->setText(QString::fromStdString(sslSettings->caFile()));
        _pemFilePathLineEdit->setText(QString::fromStdString(sslSettings->pemKeyFile()));
        _pemPassLineEdit->setText(QString::fromStdString(sslSettings->pemPassPhrase()));
        _allowInvalidHostnamesComboBox->setCurrentIndex(sslSettings->allowInvalidHostnames());
        _crlFilePathLineEdit->setText(QString::fromStdString(sslSettings->crlFile()));

        // new
        on_authModeComboBox_change(_authMethodComboBox->currentIndex());
        on_usePemFileCheckBox_toggle(_usePemFileCheckBox->isChecked());
        on_useAdvancedOptionsCheckBox_toggle(_useAdvancedOptionsCheckBox->isChecked());

        setMinimumWidth(540);
        setMinimumHeight(300);

        // Fixing issue for Windows High DPI button height is slightly bigger than other widgets 
#ifdef Q_OS_WIN
        _caFileBrowseButton->setMaximumHeight(WIN_BUTTON_HEIGHT);
        _pemFileBrowseButton->setMaximumHeight(WIN_BUTTON_HEIGHT);
        _pemPassShowButton->setMaximumHeight(WIN_BUTTON_HEIGHT);
        _crlFileBrowseButton->setMaximumHeight(WIN_BUTTON_HEIGHT);
#endif

    }

    bool SSLTab::accept()
    {
        saveSslSettings();
        return validate();
    }

    void SSLTab::useSslCheckBoxStateChange(int state)
    {
        bool isChecked = static_cast<bool>(state);
        //_authMethodLabel->setDisabled(!isChecked);
        //_authMethodComboBox->setDisabled(!isChecked);
        ////if (state)  // if SSL enabled, disable/enable conditionally; otherwise disable all widgets.
        ////{
        ////    setDisableCAfileWidgets(_acceptSelfSignedButton->isChecked());
        ////    on_useClientCertPassCheckBox_toggle(_useClientCertPassCheckBox->isChecked());
        ////}
        ////else
        ////{
        ////    setDisableCAfileWidgets(true);
        ////    _clientCertPassLineEdit->setDisabled(true);
        ////    _clientCertPassShowButton->setDisabled(true);
        ////}
        //_pemFileInfoStr->setDisabled(!isChecked);
        //_clientCertPathLineEdit->setDisabled(!isChecked);
        //_clientCertFileBrowseButton->setDisabled(!isChecked);
        //_clientCertPassLabel->setDisabled(!isChecked);
        //_useClientCertCheckBox->setDisabled(!isChecked);
        //_useClientCertPassCheckBox->setDisabled(!isChecked);
        //_allowInvalidHostnamesComboBox->setDisabled(!isChecked);
        //_crlFileLabel->setDisabled(!isChecked);
        //_crlFilePathLineEdit->setDisabled(!isChecked);
        //_crlFileBrowseButton->setDisabled(!isChecked);
    }

    void SSLTab::on_authModeComboBox_change(int index)
    {
        bool const isCaSigned = static_cast<bool>(index);
        _selfSignedInfoStr->setVisible(!isCaSigned);
        _caFileLabel->setVisible(isCaSigned);
        _caFilePathLineEdit->setVisible(isCaSigned);
        _caFileBrowseButton->setVisible(isCaSigned);
    }

    void SSLTab::on_usePemFileCheckBox_toggle(bool isChecked)
    {
        _pemFileInfoStr->setVisible(!isChecked);
        _pemFileLabel->setVisible(isChecked);
        _pemFilePathLineEdit->setVisible(isChecked);
        _pemFileBrowseButton->setVisible(isChecked);
        _pemPassLabel->setVisible(isChecked);
        _pemPassLineEdit->setVisible(isChecked);
        _pemPassShowButton->setVisible(isChecked);
        _usePemPassphraseCheckBox->setVisible(isChecked);
        if(isChecked)
        {
            setMinimumHeight(400);
        }
    }

    void SSLTab::on_useAdvancedOptionsCheckBox_toggle(bool isChecked)
    {
        _crlFileLabel->setVisible(isChecked);
        _crlFilePathLineEdit->setVisible(isChecked);
        _crlFileBrowseButton->setVisible(isChecked);
        _allowInvalidHostnamesLabel->setVisible(isChecked);
        _allowInvalidHostnamesComboBox->setVisible(isChecked);
        if(isChecked)
        {
            setMinimumHeight(460);
        }
    }

    void SSLTab::on_acceptSelfSignedButton_toggle(bool checked)
    {
        _useRootCaFileButton->setChecked(!checked);
        setDisableCAfileWidgets(checked);
    }

    void SSLTab::on_caFileBrowseButton_clicked()
    {
        QString const fileName = openFileBrowseDialog(_caFilePathLineEdit->text());
        if (!fileName.isEmpty()) {
            _caFilePathLineEdit->setText(fileName);
        }
    }

    void SSLTab::on_pemKeyFileBrowseButton_clicked()
    {
        QString const fileName = openFileBrowseDialog(_clientCertPathLineEdit->text());
        if (!fileName.isEmpty()) {
            _clientCertPathLineEdit->setText(fileName);
        }
    }

    void SSLTab::on_crlFileBrowseButton_clicked()
    {
        QString const fileName = openFileBrowseDialog(_crlFilePathLineEdit->text());
        if (!fileName.isEmpty()) {
            _crlFilePathLineEdit->setText(fileName);
        }
    }

    void SSLTab::togglePassphraseShowMode()
    {
        bool isPassword = _pemPassLineEdit->echoMode() == QLineEdit::Password;
        _pemPassLineEdit->setEchoMode(isPassword ? QLineEdit::Normal : QLineEdit::Password);
        _pemPassShowButton->setIcon(isPassword ? GuiRegistry::instance().showIcon() : GuiRegistry::instance().hideIcon());
    }

    void SSLTab::on_usePemPassphraseCheckBox_toggle(bool checked)
    {
        _pemPassLineEdit->setDisabled(!checked);
        _pemPassShowButton->setDisabled(!checked);
    }

    bool SSLTab::validate()
    {
        // Validate existence of files
        auto const resultAndFileName = checkExistenseOfFiles();
        if (!resultAndFileName.first)
        {
            QString nonExistingFile = resultAndFileName.second;
            QMessageBox errorBox;
            errorBox.critical(this, "Error", ("Error: " + nonExistingFile + " file does not exist"));
            errorBox.adjustSize();
            return false;
        }
        return true;
    }

    void SSLTab::setDisableCAfileWidgets(bool disabled)
    {
        _caFilePathLineEdit->setDisabled(disabled);
        _caFileBrowseButton->setDisabled(disabled);
    }

    std::pair<bool,QString> SSLTab::checkExistenseOfFiles() const
    {
        if (_caFilePathLineEdit->isEnabled())
        {
            if (!fileExists(_caFilePathLineEdit->text()))
            {
                return { false, "CA" };
            }
        }

        if (!_clientCertPathLineEdit->text().isEmpty())
        {
            if (!fileExists(_clientCertPathLineEdit->text()))
            {
                return{ false, "Client Certificate" };
            }
        }

        if (!_crlFilePathLineEdit->text().isEmpty())
        {
            if (!fileExists(_crlFilePathLineEdit->text()))
            {
                return{ false, "CRL (Revocation List)" };
            }
        }
        return{ true, "" };
    }

    void SSLTab::saveSslSettings() const
    {
        SslSettings *sslSettings = _settings->sslSettings();
        sslSettings->enableSSL(_useSslCheckBox->isChecked());
        sslSettings->setCaFile(QtUtils::toStdString(_caFilePathLineEdit->text()));
        sslSettings->setPemKeyFile(QtUtils::toStdString(_clientCertPathLineEdit->text()));
        sslSettings->setPemPassPhrase(QtUtils::toStdString(_clientCertPassLineEdit->text()));
        sslSettings->setPemKeyEncrypted(_useClientCertPassCheckBox->isChecked());
        sslSettings->setAllowInvalidCertificates(_acceptSelfSignedButton->isChecked());
        sslSettings->setAllowInvalidHostnames(static_cast<bool>(_allowInvalidHostnamesComboBox->currentIndex()));
        sslSettings->setCrlFile(QtUtils::toStdString(_crlFilePathLineEdit->text()));
    }

    QString SSLTab::openFileBrowseDialog(const QString& initialPath)
    {
        QString filePath = initialPath;
        // If user has previously selected a file, initialize file dialog with that file's 
        // path and name; otherwise, use user's home directory.
        if (filePath.isEmpty())
        {
            filePath = QDir::homePath();
        }
        QString fileName = QFileDialog::getOpenFileName(this, tr("Choose File"), filePath);
        return QDir::toNativeSeparators(fileName);
    }

    bool SSLTab::eventFilter(QObject * watched, QEvent * event)
    {
        QLabel const * const label = qobject_cast<QLabel *>(watched);
        if (label && event->type() == QEvent::MouseButtonPress)
        {
            if (label == _usePemFileCheckBoxLabel)
            {
                _usePemFileCheckBox->toggle();
            }
            else if (label == _useAdvancedOptionsLabel)
            {
                _useAdvancedOptionsCheckBox->toggle();
            }
        }
        return QObject::eventFilter(watched, event);
    }
}

