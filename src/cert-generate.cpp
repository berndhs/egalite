#include "cert-generate.h"
#include <QtCrypto>
#include <QFile>
#include <QDateTime>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>


namespace egalite 
{

CertGenerate::CertGenerate (QWidget *parent)
  :QDialog (parent),
   showCooked (false)  
{
  ui.setupUi (this);
  tagCN = tr("1.Common Name");
  tagEmail = tr("2.e-mail");
  tagCountry = tr("3.Country");
  tagState = tr ("4.State/Province");
  tagLocality = tr("5.Locality");
  tagCompany = tr("6.Organization");
  tagUnit = tr ("7.Oranization Unit");
  tagDays = tr ("8.Valid for Days");
  stringDataMap[tagCN] = tr ("Your Name");
  stringDataMap[tagEmail]      = QString ("");
  stringDataMap[tagCountry]    = QString ("");
  stringDataMap[tagState]      = QString ("");
  stringDataMap[tagLocality]   = QString ("");
  stringDataMap[tagCompany]    = QString ("");
  stringDataMap[tagUnit]       = QString ("");
  stringDataMap[tagDays]       = QString ("365");
  stringDataModel = new QStandardItemModel (this);
  FillModel (stringDataModel, stringDataMap);
  ui.dataTable->setModel (stringDataModel);
  connect (ui.generateButton, SIGNAL (clicked()), this, SLOT (Generate ()));
  connect (ui.cancelButton, SIGNAL (clicked()), this, SLOT (Cancel ()));
  connect (ui.usenowButton, SIGNAL (clicked()), this, SLOT (UseNow()));
  connect (ui.restartButton, SIGNAL (clicked()), this, SLOT (Restart()));
  connect (ui.exportButton, SIGNAL (clicked()), this, SLOT (Export ()));
  connect (&keyGen, SIGNAL (finished ()), this, SLOT (FinishGenerate ()));
}

void
CertGenerate::FillModel (QStandardItemModel *model,
                     std::map < QString, QString > & data)
{
  QStandardItem *keyItem, *valItem;
  QString        key,      val;
  std::map <QString, QString> :: iterator mit;
  for (mit = data.begin(); mit != data.end(); mit++) {
    key = mit->first;
    val = mit->second;
    keyItem = new QStandardItem (key);
    keyItem->setEditable (false);
    valItem = new QStandardItem (val);
    valItem->setEditable (true);
    QList <QStandardItem*> rowList;
    rowList << keyItem << valItem;
    model->appendRow (rowList);
  }
}

void
CertGenerate::FillMap (std::map <QString, QString> & data,
                   QStandardItemModel *model)
{
  int nrows = model->rowCount();
  QStandardItem * keyItem;
  QStandardItem * valItem;
  for (int r = 0; r < nrows; r++) {
    keyItem = model->item (r,0);
    valItem = model->item (r,1);
    if (keyItem && valItem) {
      data[keyItem->text()] = valItem->text();
    }
  }
}

void
CertGenerate::Dialog ()
{
  if ( ! QCA::isSupported ("cert")) {
    QMessageBox nosupport;
    nosupport.setText (tr("Your System does not support generating certificates"));
    nosupport.exec ();
    return;
  }
  ui.certEdit->hide();
  ui.exportButton->hide();
  ui.usenowButton->hide();
  ui.passwordLabel->show();
  ui.passwordEdit->show();
  ui.generateButton->show();
  ui.dataTable->show();
  show ();
}

void
CertGenerate::Restart ()
{
  QTimer::singleShot (250,this, SLOT (Dialog()));
}

void
CertGenerate::Generate ()
{ 

  FillMap (stringDataMap, stringDataModel);

  ui.certEdit->clear ();
  ui.certEdit->appendPlainText (QString("generating RSA private key"));
  update ();
  genTimer = new QTimer (this);
  connect (genTimer, SIGNAL (timeout()), this, SLOT (GenTimeout()));
  genTimer->start (1000);
  timeCount = 0;
  ui.certEdit->show();
  keyGen.setBlockingEnabled (false);
  keyGen.createRSA (4096);
}

void
CertGenerate::GenTimeout ()
{
  static int timeCount (0);
  timeCount++;
  qDebug () << " key gen ticker count " << timeCount;
  QString num = QString::number (timeCount);
  ui.certEdit->setPlainText (tr ("generating key...") + num);
  if (timeCount > 120) {
    FinishGenerate ();
  }
}

void
CertGenerate::FinishGenerate ()
{
  qDebug () << " done generating key";
  ui.certEdit->setPlainText (tr("Finished generating key"));
  genTimer->stop ();
  
  QCA::PrivateKey newkey = keyGen.key ();
  qDebug () << " key null says " << newkey.isNull();
  if (newkey.isNull ()) {
    qDebug () << " bad key gen";
    ui.certEdit->appendPlainText (QString ("key generation failed"));
    return;
  }
  QCA::CertificateOptions  certOpts;
  certOpts.setAsCA ();
  password = ui.passwordEdit->text();
  certOpts.setChallenge (password);
  QString daystring = QDateTime::currentDateTime()
                      .toString ("yyyyMMddhhmmss0000");
  QCA::BigInteger daynum (daystring);
  QCA::BigInteger microsec (QTime::currentTime().msec());
  certOpts.setSerialNumber (daynum += microsec);
  certOpts.setFormat (QCA::PKCS10);

  QCA::CertificateInfo   info;
  info.insert (QCA::Country, stringDataMap[tagCountry]);
  info.insert ( QCA::CommonName,stringDataMap[tagCN]);
  info.insert ( QCA::State, stringDataMap[tagState]);
  info.insert ( QCA::Email, stringDataMap[tagEmail]);
  info.insert ( QCA::Locality, stringDataMap[tagLocality]);
  info.insert ( QCA::Organization, stringDataMap[tagCompany]);
  info.insert ( QCA::OrganizationalUnit, stringDataMap[tagUnit]);

  certOpts.setInfo (info);
  certOpts.setOCSPLocations (QStringList());
  certOpts.setIssuerLocations (QStringList());
  int days = stringDataMap[tagDays].toInt();
  QDateTime now = QDateTime::currentDateTime();
  certOpts.setValidityPeriod (now, now.addDays (days));

  QCA::Certificate newcert (certOpts, newkey);
  newcertPEM = newcert.toPEM ();
  newkeyPEM  = newkey.toPEM ();
  qDebug () << " cert feature supported? " << QCA::isSupported ("cert");
  qDebug () << " made new cert";

  ui.dataTable->hide();
  ui.generateButton->hide();
  ui.passwordLabel->hide();
  ui.passwordEdit->hide();
  ui.certEdit->show();
  ui.exportButton->show();
  showCooked = true;
  storedCert = QSslCertificate (newcertPEM.toAscii());
  if (storedCert.isValid ()) {
    ui.usenowButton->show();
  }
  ShowCertDetails ();
}

void
CertGenerate::ShowCertDetails ()
{
  QSslCertificate cert = storedCert;
  QStringList lines;
  if (showCooked) {
    ui.certEdit->setReadOnly (true);
    lines  << tr ("Serial Number: %1")
              .arg (QString (cert.serialNumber()))
        << tr("Organization: %1")
              .arg(cert.subjectInfo(QSslCertificate::Organization))
        << tr("Subunit: %1")
              .arg(cert.subjectInfo(QSslCertificate::OrganizationalUnitName))
        << tr("Country: %1")
              .arg(cert.subjectInfo(QSslCertificate::CountryName))
        << tr("Locality: %1")
              .arg(cert.subjectInfo(QSslCertificate::LocalityName))
        << tr("State/Province: %1")
              .arg(cert.subjectInfo(QSslCertificate::StateOrProvinceName))
        << tr("Common Name: %1")
              .arg(cert.subjectInfo(QSslCertificate::CommonName))
        << tr("Valid from: %1")
              .arg(cert.effectiveDate().toString ())
        << tr("Valid to: %1")
              .arg(cert.expiryDate().toString ())
        << tr("Currently valid: %1").arg ((cert.isValid() ? "Yes" : "No"))
        << QString("------------")
        << tr("Issuer Organization: %1")
              .arg(cert.issuerInfo(QSslCertificate::Organization))
        << tr("Issuer Unit Name: %1")
              .arg(cert.issuerInfo(QSslCertificate::OrganizationalUnitName))
        << tr("Issuer Country: %1")
              .arg(cert.issuerInfo(QSslCertificate::CountryName))
        << tr("Issuer Locality: %1")
              .arg(cert.issuerInfo(QSslCertificate::LocalityName))
        << tr("Issuer State/Province: %1")
              .arg(cert.issuerInfo(QSslCertificate::StateOrProvinceName))
        << tr("Issuer Common Name: %1")
              .arg(cert.issuerInfo(QSslCertificate::CommonName));
  } else {
    ui.certEdit->setReadOnly (false);
    lines << cert.toPem ();
  }
  QStringList::iterator lit;
  ui.certEdit->clear ();
  for (lit = lines.begin(); lit != lines.end(); lit++) {
    ui.certEdit->appendPlainText (*lit);
  }
}

void
CertGenerate::Export ()
{
  QString pathDefault ("./");
  QString certDefault = stringDataMap[tagCN]
                        + QString ("-cert.pem");
  QString certfileName = QFileDialog::getSaveFileName (this,
                           tr("Save Certificate in File"),
                           pathDefault + certDefault);
  if (certfileName.length () == 0) {
    return;
  }
  QFile certfile (certfileName);
  certfile.open (QFile::WriteOnly);
  certfile.write (newcertPEM.toAscii());
  certfile.close ();
  certfile.open (QFile::ReadOnly);
  QSslCertificate cert (newcertPEM.toAscii());
  certfile.close ();
  storedCert = cert;

  QString keyDefault = stringDataMap[tagCN]
                        + QString ("-cert.pem");
qDebug () << " default key file name " << pathDefault + keyDefault;
  QString keyfileName = QFileDialog::getSaveFileName (this,
                           tr("Save Key in File"),
                          pathDefault + keyDefault);
  if (certfileName.length () == 0) {
    return;
  }
  QFile keyfile (keyfileName);
  keyfile.open (QFile::WriteOnly);
  keyfile.write (newkeyPEM.toAscii());
  keyfile.close ();
}

void
CertGenerate::Cancel ()
{
  done (0);
}

void
CertGenerate::UseNow ()
{
  done (1);
qDebug () << " new cert PEM " << newcertPEM.left(100);
  emit NewCertificate (stringDataMap[tagCN],
                       password,
                       newkeyPEM,
                       newcertPEM);
}

} // namespace

