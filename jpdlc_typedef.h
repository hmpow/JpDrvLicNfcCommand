#ifndef JP_DRV_LIC_NFC_COMMAND_TYPEDEF_H
#define JP_DRV_LIC_NFC_COMMAND_TYPEDEF_H

/* デバッグモード設定 */
//#define DLC_LAYER_DEBUG
#define DLC_LAYER_SHOW_COMMUNICATION_FRAME

#include <vector>
#include <array>
#include <stdint.h>

//特別な意味のある物は型指定でバグ防止
typedef uint8_t  type_data_byte;
typedef uint16_t type_tag;        //16bitタグ対応
typedef uint8_t  type_short_efid; //8bitショートEF識別子
typedef uint16_t type_full_efid;  //16bitEF識別子

typedef std::array<type_data_byte, 4> type_PIN;

typedef struct _jpdlc_status_response{
    uint8_t sw1;
    uint8_t sw2;
} JPDLC_STATUS_RESPONSE;

typedef struct{
    type_tag tag;
    uint16_t len;
    std::vector<type_data_byte> value;
} JPDLC_TLV_SET;

typedef enum _card_status{
    JPDLC_STATUS_OK = 0,
    JPDLC_STATUS_WARNING,
    JPDLC_STATUS_ERROR
} JPDLC_CARD_STATUS;

typedef enum _isset_pin_status{
    PIN_NOT_SET = 0,
    PIN_SET,
    PIN_ERROR
} JPDLC_ISSET_PIN_STATUS;

typedef struct _expiration_data{
    uint16_t yyyy;
    uint8_t  m;
    uint8_t  d;
} JPDLC_EXPIRATION_DATA;


#endif // JP_DRV_LIC_NFC_COMMAND_TYPEDEF_H