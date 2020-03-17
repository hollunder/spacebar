#include "chatlistmodel.h"

#include <QStandardPaths>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QThread>

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/AccountSet>
#include <TelepathyQt/PendingChannelRequest>

#include <KPeople/PersonData>

#include "Global.h"

ChatListModel::ChatListModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_database(new Database(this))
    , m_chats(m_database->chats())
    , m_mapper(new ContactMapper(this))
{
    m_mapper->performInitialScan();
    connect(m_database, &Database::messagesChanged, this, &ChatListModel::fetchChats);
    connect(m_mapper, &ContactMapper::contactsChanged, this, [this](const QVector<QString> &affectedNumbers) {
        qDebug() << "New data for" << affectedNumbers;
        for (const auto &uri : affectedNumbers) {
            for (int i = 0; i < m_chats.count(); i++) {
                if (m_chats.at(i).phoneNumber == uri) {
                    auto row = this->index(i);
                    this->dataChanged(row, row, {Role::DisplayNameRole});
                }
            }
        }
    });

    // Set up sms account
    QEventLoop loop;
    Tp::AccountManagerPtr manager = Tp::AccountManager::create();
    Tp::PendingReady *ready = manager->becomeReady();
    QObject::connect(ready, &Tp::PendingReady::finished, &loop, &QEventLoop::quit);
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    const Tp::AccountSetPtr accountSet = manager->validAccounts();
    const auto accounts = accountSet->accounts();
    for (const Tp::AccountPtr &account : accounts) {
        static const QStringList supportedProtocols = {
            QLatin1String("ofono"),
            QLatin1String("tel"),
        };
        if (supportedProtocols.contains(account->protocolName())) {
            m_simAccount = account;
            break;
        }
    }
}

QHash<int, QByteArray> ChatListModel::roleNames() const
{
    return {
        { Role::DisplayNameRole, "displayName" },
        { Role::PhoneNumberRole, "phoneNumber" },
        { Role::LastContactedRole, "lastContacted" },
        { Role::UnreadMessagesRole, "unreadMessages" }
    };
}

QVariant ChatListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_chats.count()) {
        return false;
    }

    switch(role) {
    case DisplayNameRole: {
        auto *person = new KPeople::PersonData(m_mapper->uriForNumber(m_chats.at(index.row()).phoneNumber));
        QString name = person->name();
        delete person;
        return name;
    }
    case PhoneNumberRole:
        return m_chats.at(index.row()).phoneNumber;
    case LastContactedRole:
        return m_chats.at(index.row()).lastContacted;
    case UnreadMessagesRole:
        return m_chats.at(index.row()).unreadMessages;
    };

    return {};
}

int ChatListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_chats.count();
}

void ChatListModel::startChat(const QString &phoneNumber)
{
    if (m_simAccount.isNull()) {
        qCritical() << "Unable to get SIM account;" << "is the telepathy-ofono or telepathy-ring backend installed?";
    }

    auto *pendingRequest = m_simAccount->ensureTextChat(phoneNumber);
    QObject::connect(pendingRequest, &Tp::PendingChannelRequest::finished, pendingRequest, [=](){
        if (pendingRequest->isError()) {
            qWarning() << "Error when requesting channel" << pendingRequest->errorMessage();
        }
        if (pendingRequest->channelRequest()) {
            if (pendingRequest->channelRequest()->channel()) {
                auto channel = pendingRequest->channelRequest()->channel();
                channel->becomeReady();
                qDebug() << "channel is ready" << channel->isReady();
            }
        }
    });
}

void ChatListModel::fetchChats()
{
    beginResetModel();
    m_chats = m_database->chats();
    endResetModel();
}
