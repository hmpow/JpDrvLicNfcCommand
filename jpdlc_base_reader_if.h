#ifndef JP_DRV_LIC_NFC_COMMAND_READER_IF_H
#define JP_DRV_LIC_NFC_COMMAND_READER_IF_H

#include "jpdlc_typedef.h"

//RCS660S以外も使えるようにI/Fクラス作っておく

//カードリーダーをインクルード
#include "rcs660s_app_if.h"

#define READER_MAX_COMMAND_LEN (uint16_t)250
#define READER_MAX_RES_LEN     (uint16_t)250

void setReaderInstance(Rcs660sAppIf *);
std::vector<type_data_byte> _nfcTransceive(const std::vector<type_data_byte> );

std::vector<type_data_byte> _nfcTransceive_Stub(const std::vector<type_data_byte> );


#endif // JP_DRV_LIC_NFC_COMMAND_READER_IF_H
