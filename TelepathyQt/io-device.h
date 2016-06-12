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

#ifndef _TelepathyQt_io_device_h_HEADER_GUARD_
#define _TelepathyQt_io_device_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Global>

#include <QIODevice>

namespace Tp
{

class TP_QT_EXPORT IODevice : public QIODevice
{
    Q_OBJECT
public:
    explicit IODevice(QObject *parent = 0);
    ~IODevice();
    bool isSequential() const;
    qint64 bytesAvailable() const;

protected:
    qint64 readData(char *data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);

private:
    class Private;
    friend class Private;
    Private *mPriv;
};

} // namespace Tp

#endif // _TelepathyQt_io_device_h_HEADER_GUARD_
