/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2016 Niels Ole Salscheider <niels_ole@salscheider-online.de>
 * @copyright Copyright (C) 2016 Alexandr Akulich <akulichalexander@gmail.com>
 * @license LGPL 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <TelepathyQt/IODevice>

#include "TelepathyQt/_gen/io-device.moc.hpp"

namespace Tp
{

struct TP_QT_NO_EXPORT IODevice::Private
{
    QByteArray data;
};

/**
 * \class IODevice
 * \ingroup utils
 * \headerfile <TelepathyQt/IODevice>
 *
 * \brief The IODevice class represents a buffer with independent read-write.
 *
 * QBuffer has one position pointer, so when we write data, the position
 * pointer points to the end of the buffer and no bytes can be read.
 *
 * This class is interesting for all CMs that use a library that accepts a
 * QIODevice for file transfers.
 *
 * Note: This class belongs to the service library.
 */

IODevice::IODevice(QObject *parent) :
    QIODevice(parent),
    mPriv(new Private)
{
}

IODevice::~IODevice()
{
    delete mPriv;
}

qint64 IODevice::bytesAvailable() const
{
    return QIODevice::bytesAvailable() + mPriv->data.size();
}

/**
 * Returns the number of bytes that are available for reading.
 *
 * \return the number of bytes that are available for reading.
 */
bool IODevice::isSequential() const
{
    return true;
}

qint64 IODevice::readData(char *data, qint64 maxSize)
{
    qint64 size = qMin<qint64>(mPriv->data.size(), maxSize);
    memcpy(data, mPriv->data.constData(), size);
    mPriv->data.remove(0, size);
    return size;
}

/**
 * Writes the data to the buffer.
 *
 * Writes up to \a maxSize bytes from \a data to the buffer.
 * If maxSize is not a zero, emits readyRead() and bytesWritten() signals.
 *
 * \param data The data to write.
 * \param maxSize The number for bytes to write.
 * \return The number of bytes that were written.
 */
qint64 IODevice::writeData(const char *data, qint64 maxSize)
{
    if (maxSize <= 0) {
        return 0;
    }

    mPriv->data.append(data, maxSize);
    Q_EMIT bytesWritten(maxSize);
    Q_EMIT readyRead();
    return maxSize;
}

}
