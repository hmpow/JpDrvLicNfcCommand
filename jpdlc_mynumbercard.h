/**
 * @file jpdlc_mynumbercard.h
 * @brief マイナ免許証のコマンドクラス
 * @note  マイナ免許証を使いたい場合にincludeする
 */

#ifndef JP_DRV_LIC_NFC_COMMAND_MYNUMBERCARD_H
#define JP_DRV_LIC_NFC_COMMAND_MYNUMBERCARD_H

#include "jpdlc_base.h"

/***************/
/* マイナ免許証 */
/***************/

typedef enum _jpdlc_mynumbercard_current_selected {
    NOT_SELECTED = 0,
    ELF,
    EXE,
    INS,
    INS_IRF = 10, 
    INS_WEF01,
    INS_WEF02,
    INS_WEF03,
} JPDLC_MYNUMBERCARD_CURRENT_SELECTED;

/* AIDなどは定数宣言のためcppファイルで定義 */

class JpDrvLicNfcCommandMynumber : public JpDrvLicNfcCommandBase{
    public:

    JpDrvLicNfcCommandMynumber();

    JPDLC_ISSET_PIN_STATUS issetPin(void) override;
    bool isDrvLicCard(void) override;
    JPDLC_EXPIRATION_DATA getExpirationData(void) override;
    uint8_t getRemainingCount(void) override;
    bool executeVerify(type_PIN) override;

    private:
    JPDLC_MYNUMBERCARD_CURRENT_SELECTED current_selected;
    JPDLC_CARD_STATUS selectInsAid();
};

#endif // JP_DRV_LIC_NFC_COMMAND_MYNUMBERCARD_H