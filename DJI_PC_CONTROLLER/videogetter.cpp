#include "videogetter.h"

#include <chrono>

#include <QDebug>
#include <QFile>

#include <opencv2/highgui.hpp>
#include <opencv2/core/opengl.hpp>
#include <opencv2/cudacodec.hpp>

VideoGetter::VideoGetter()
    : socket_type("VIDEO_YUV_TYPE"),
      ip("0.0.0.0"), port(1212)
{
    socket = nullptr;
}

VideoGetter::~VideoGetter()
{
    if (socket != nullptr){
        delete socket;
    }
}

void VideoGetter::setAddress(QString ip, quint16 port)
{
    this->ip = ip;
    this->port = port;
}

void VideoGetter::start()
{
    socket = new QTcpSocket();
    socket->connectToHost(ip, port, QTcpSocket::ReadWrite,
                         QAbstractSocket::IPv4Protocol);

    if (socket->waitForConnected(3000)){
        socket->write(socket_type.toLocal8Bit());
        socket->write("\n");
        socket->flush();
    }

    readLoop();

    socket->close();
}

void VideoGetter::interrupt()
{
    socket->close();
    emit finished();
}

void VideoGetter::readLoop()
{
    using std::chrono::high_resolution_clock;
    using std::chrono::milliseconds;
    using std::chrono::duration_cast;

    qDebug() << "Socket readable: " << socket->isReadable();
    qDebug() << "Socket is open: " << socket->isOpen();
    qDebug() << "ReadLoop\n";

    auto start_time = high_resolution_clock::now();

    while (socket->isOpen()){
        if (socket->waitForReadyRead()){


            qDebug() << "Available: " << socket->bytesAvailable();
            int frameNumber = readInt(socket);
            qDebug() << "Frame number: " << frameNumber;

            /*if (frameNumber > 1000){
                socket->close();
            }*/

            uint frameByteSize = readInt(socket);
            qDebug() << "Frame byte size: " << frameByteSize;

            QByteArray message;
            message.resize(frameByteSize);
            uint received = 0;

            while (received != frameByteSize){
                uint remaining = frameByteSize - received;
                if (socket->waitForReadyRead()){
                    uint readed = socket->read(message.data() + received,
                                                       remaining);
                    received += readed;
                    //qDebug() << "readed: " << readed << " received: " << received;
                }
            }

            emit gotFrame(message, frameNumber);
        }
    }
    auto finish_time = high_resolution_clock::now();
    qDebug() << "Receiving time: " << duration_cast<milliseconds>(finish_time - start_time)
                                      .count() << "ms";
}

int VideoGetter::readInt(QTcpSocket *socket)
{
    int result = 0;

    while (socket->bytesAvailable() < 4)
        socket->waitForReadyRead();

    unsigned char intData[4] = {0, 0, 0, 0};
    socket->read((char*)intData, 4);

    qDebug() << "Readed int\nAvailable: " << socket->bytesAvailable();
    qDebug() << "Bytes: " << (int)intData[0] << " " << (int)intData[1] << " "
               << (int)intData[2] << " " << (int)intData[3];

    result = intData[0] << 24 |
             intData[1] << 16 |
             intData[2] << 8  |
             intData[3];

    return result;
}
