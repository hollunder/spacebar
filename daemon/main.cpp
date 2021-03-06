// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: LicenseRef-KDE-Accepted-GPL

#include <QCoreApplication>
#include <TelepathyQt/Types>

#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/ConnectionFactory>

#include "global.h"
#include "channellogger.h"


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(SL("KDE"));
    QCoreApplication::setOrganizationDomain(SL("kde.org"));
    QCoreApplication::setApplicationName(SL("Spacebar"));

    Tp::registerTypes();

    // Create registrar
    auto accountFactory = Tp::AccountFactory::create(QDBusConnection::sessionBus(),
        Tp::Features({Tp::Account::FeatureCore}));
    auto connectionFactory = Tp::ConnectionFactory::create(QDBusConnection::sessionBus(),
        Tp::Features({Tp::Connection::FeatureCore, Tp::Connection::FeatureSelfContact, Tp::Connection::FeatureConnected}));
    auto channelFactory = Tp::ChannelFactory::create(QDBusConnection::sessionBus());
    channelFactory->addFeaturesForTextChats(Tp::Features({Tp::TextChannel::FeatureCore, Tp::TextChannel::FeatureMessageQueue, Tp::TextChannel::FeatureMessageSentSignal}));

    auto registrar = Tp::ClientRegistrar::create(accountFactory, connectionFactory, channelFactory);

    // Create observer
    auto handler = Tp::SharedPtr<ChannelLogger>(new ChannelLogger());
    registrar->registerClient(handler, SL("SpaceObserver"));

    QCoreApplication::exec();
}
