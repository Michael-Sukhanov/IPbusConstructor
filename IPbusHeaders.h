

#ifndef IPBUSHEADERS_H
#define IPBUSHEADERS_H

#include <QtGlobal>
#include <QtEndian>
#include <QtNetwork/QtNetwork>

enum PacketType {control = 0, status = 1, resend = 2};
struct PacketHeader {
    quint32 PacketType      :  4,
            ByteOrder       :  4,
            PacketID        : 16,
            Rsvd            :  4,
            ProtocolVersion :  4;
    PacketHeader(enum PacketType t = status, quint16 id = 0) {
        PacketType = t;
        ByteOrder = 0xf;
        PacketID = id;
        Rsvd = 0;
        ProtocolVersion = 2;
    }
    PacketHeader(quint8 t, quint16 id = 0) {
        PacketType = t;
        ByteOrder = 0xf;
        PacketID = id;
        Rsvd = 0;
        ProtocolVersion = 2;
    }
    PacketHeader(const quint32 &word) {memcpy(this, &word, 4);}//memcpy копирует количество байт n (в нашем случае 4) из строки str1 в строку this
    operator quint32() {return *reinterpret_cast<quint32 *>(this);}
    operator quint32() const {return *reinterpret_cast<const quint32 *>(this);}
};

enum TransactionType {
    read                  = 0,
    write                 = 1,
    nonIncrementingRead   = 2,
    nonIncrementingWrite  = 3,
    RMWbits               = 4,
    RMWsum                = 5,
    configurationRead     = 6,
    configurationWrite    = 7
};
struct TransactionHeader {
    quint32 InfoCode        :  4,
            TypeID          :  4,
            Words           :  8,
            TransactionID   : 12,
            ProtocolVersion :  4;
    TransactionHeader(TransactionType t, quint8 nWords, quint16 id = 0,quint8 IC = 0xf) {
        InfoCode = IC;
        TypeID = t;
        Words = nWords;
        TransactionID = id;
        ProtocolVersion = 2;
    }
    TransactionHeader(quint8 t, quint8 nWords, quint16 id = 0,quint8 IC = 0xf) {
        InfoCode = IC;
        TypeID = t;
        Words = nWords;
        TransactionID = id;
        ProtocolVersion = 2;
    }
    TransactionHeader(const quint32 &word) {memcpy(this, &word, 4);}
    operator quint32() {return *reinterpret_cast<quint32 *>(this);}
    QString infoCodeString() {
        switch (InfoCode) {
            case 0x0: return "Successfull request";
            case 0x1: return "Bad header";
            case 0x4: return "IPbus read error";
            case 0x5: return "IPbus write error";
            case 0x6: return "IPbus read timeout";
            case 0x7: return "IPbus write timeout";
            case 0xf: return "outbound request";
            default : return "unknown Info Code";
        }
    }
    QString typeIDString(){
        switch (TypeID) {
			case read:                 return "Read             ";
			case write:                return "Write            ";
			case nonIncrementingRead:  return "Read  (non-incr.)";
			case nonIncrementingWrite: return "Write (non-incr.)";
			case RMWbits:              return "RMWbits          ";
			case RMWsum:               return "RMWsum           ";
			default:                   return " !!! unknown !!! ";
        }
    }
};

//struct StatusPacket {
//    PacketHeader header = qToBigEndian(quint32(PacketHeader(status))); //0x200000F1: {0xF1, 0, 0, 0x20} -> {0x20, 0, 0, 0xF1}
//    quint32 MTU = 0,
//            nResponseBuffers = 0,
//            nextPacketID = 0;
//    quint8  trafficHistory[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
//    quint32 controlHistory[8] = {0,0,0,0, 0,0,0,0};
//};

struct Parameter {
    quint32 address;
    quint8 bitwidth,
           bitshift,
           channelsInterval; //for PM channels parameters
    Parameter(): address(0), bitwidth(0), bitshift(0), channelsInterval(0) {}
    Parameter(quint32 addr, quint8 w, quint8 sh, quint8 chw): address(addr), bitwidth(w), bitshift(sh), channelsInterval(chw) {}
};

#endif // IPBUSHEADERS_H
