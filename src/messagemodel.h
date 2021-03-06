// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: LicenseRef-KDE-Accepted-GPL

#pragma once

#include <QObject>
#include <QAbstractListModel>

#include <KPeople/PersonData>

#include <TelepathyQt/Channel>

#include "database.h"
class AsyncDatabase;

class MessageModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString phoneNumber READ phoneNumber NOTIFY phoneNumberChanged)
    Q_PROPERTY(KPeople::PersonData *person READ person NOTIFY personChanged)
    Q_PROPERTY(bool isReady READ isReady NOTIFY isReady NOTIFY isReadyChanged)

public:
    enum Role {
        TextRole = Qt::UserRole + 1,
        DateRole,
        TimeRole,
        SentByMeRole,
        ReadRole,
        DeliveredRole,
        IdRole
    };
    Q_ENUM(Role)

    explicit MessageModel(AsyncDatabase *database,
                          const QString &phoneNumber,
                          const Tp::TextChannelPtr& channel,
                          const QString &personUri = {},
                          QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &index = {}) const override;

    KPeople::PersonData *person() const;

    QString phoneNumber() const;

    /**
     * @brief Adds a message to the model and the database.
     * Can be used for example when a new message is received.
     * @param message
     */
    void addMessage(const Message &message);

    /**
     * @brief sends a message with the specified text,
     * and adds it to the model and database by calling addMessage(const QString&)
     * @param text
     */
    Q_INVOKABLE void sendMessage(const QString &text);

    /**
     * @brief marks a message as read by calling the respective database function
     */
    Q_INVOKABLE void markMessageRead(const int id);

    bool isReady() const;

private:
    AsyncDatabase *m_database;
    QVector<Message> m_messages;
    Tp::TextChannelPtr m_channel;

    // properties
    QString m_phoneNumber;
    KPeople::PersonData *m_personData;
    bool m_isReady = false;

signals:
    void phoneNumberChanged();
    void personChanged();
    void isReadyChanged();
};
