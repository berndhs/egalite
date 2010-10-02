#ifndef CERT_GENERATE_H
#define CERT_GENERATE_H

#include "ui_cert-input.h"
#include <QSslCertificate>
#include <QtCrypto>
#include <QTimer>

class QStandardItemModel ;

namespace egalite
{

class CertGenerate : public QDialog
{
Q_OBJECT

public:

  CertGenerate (QWidget *parent = 0);

  void ShowCertDetails ();

public slots:

  void Dialog (bool reset = true);
  void ReDialog ();
  void Generate ();
  void FinishGenerate ();
  void Cancel ();
  void GenTimeout ();

private slots:
  
  void Restart ();
  void Export ();
  void UseNow ();

signals:

  void NewCertificate (QString name,
                       QString password,
                       QString keyPEM,
                       QString certPEM,
                       bool    saveDirectly);

private:

  void FillModel (QStandardItemModel * model, 
                  const QStringList & headers,
                  const std::map <QString, QString> & data);
  void FillMap  (std::map <QString, QString> & data,
                 QStandardItemModel * model);
  void ResetMap (std::map <QString, QString> & data);

  Ui_CertificateInput    ui;
  QSslCertificate       storedCert;
  QString               newcertPEM;
  QString               newkeyPEM;
  QString               password;
  bool                  showCooked;
  QCA::KeyGenerator     keyGen;
  QTimer     *genTimer;
  int        timeCount;

  QStandardItemModel          * stringDataModel;
  QStringList                   modelHeaders;
  std::map <QString, QString>   stringDataMap;

  QString  tagCN;
  QString  tagEmail;
  QString  tagCountry;
  QString  tagState;
  QString  tagLocality;
  QString  tagCompany;
  QString  tagUnit;
  QString  tagDays;
};

} // namespace


#endif

