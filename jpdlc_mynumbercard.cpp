/**
 * @file jpdlc_mynumbercard.cpp
 * @brief マイナ免許証のコマンドクラス実装
 */

#include "jpdlc_mynumbercard.h"

/***************/
/* マイナ免許証 */
/***************/

const type_data_byte AID_ELF[] = { 0xA0,0x00,0x00,0x02,0x31,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
const type_data_byte AID_EXE[] = { 0xA0,0x00,0x00,0x02,0x31,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
const type_data_byte AID_INS[] = { 0xA0,0x00,0x00,0x02,0x31,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

const type_full_efid FULL_FEID_WEF01_PINSETTING   = 0x001A; //PIN設定有無

const type_full_efid FULL_FEID_IEF01_PIN          = 0x0006; //PIN1の短縮EF 別紙3記載　⇒ こちらが正 63 CA (残り10回)が返ってくる

//const type_full_efid FULL_FEID_IEF01_PIN_BETTEN   = 0x0002; //PIN1の短縮EF 別添3-1記載　⇒ 誤記っぽい 6A 82 (アクセス対象ファイル無し)が返ってくる

const type_full_efid FULL_FEID_WEF02_LICENSEDATA  = 0x001B; //免許情報
const type_full_efid FULL_FEID_WEF03_SECURITYDATA = 0x001C; //電子署名や発行者識別情報など　使用しない

const uint16_t       LE_OF_WEF01            = 3;      //T,L,V 各1byte
const type_tag       TAG_OF_WEF01           = 0x00C1; //PIN設定

const type_tag       TAG_OF_EXPIRATION_DATA = 0x00C5; //有効期限情報

const type_data_byte REIWA_CODE             = 0x05;   //免許証仕様上の令和の識別コード

const uint8_t        NO_OFFSET              = 0x00;
const type_data_byte WEF01_PIN_SETTING_ON   = 0x01;   //仕様書指定値 PIN設定ありの場合
const type_data_byte WEF01_PIN_SETTING_OFF  = 0x00;   //仕様書指定値 PIN設定無しの場合


/* コンストラクタ */

JpDrvLicNfcCommandMynumber::JpDrvLicNfcCommandMynumber(){
    current_selected = NOT_SELECTED;
}

/* public */


/**
 * @brief  タッチされたNFC-TypeBカードがマイナ免許証であるか確認する
 * @param  なし
 * @retval true  : マイナ免許証である
 * @retval false : マイナ免許証ではない
 * @note   カードリーダがType-Bを捕捉し、通信確立された状態で実行する
 *  　　　　マイナ免許証ならあるはずのAIDをお試し SELECT し、成功するか確認する
 *         ELF-AID と 実行モジュール-AID は選択できないようのでスキップする
 */
bool JpDrvLicNfcCommandMynumber::isDrvLicCard(void){

    JPDLC_CARD_STATUS card_status = JPDLC_STATUS_ERROR;

#ifdef DLC_LAYER_DEBUG
    //AID_ELF があるか → 6a 82 になる デバッグのため残しておくが判定から除外
    printf("isDrvLicCard マイナ免許 AID_ELF を SELECT\r\n");
   
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_ELF, sizeof(AID_ELF)/sizeof(AID_ELF[0]))
        )
    );

    //AID_EXE があるか → 6a 82 になる デバッグのため残しておくが判定から除外
    printf("isDrvLicCard マイナ免許 AID_EXE を SELECT\r\n");

    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_EXE, sizeof(AID_EXE)/sizeof(AID_EXE[0]))
        )
    );
#endif

    //AID_INS があるか → 90 00 になる

#ifdef DLC_LAYER_DEBUG
    printf("isDrvLicCard マイナ免許 AID_INS を SELECT\r\n");
#endif

    card_status = selectInsAid();
    if(card_status == JPDLC_STATUS_ERROR){
        return false;
    }

    //ここまでreturn されなければOK
    return true;
}

/**
 * @brief  マイナ免許証にPINが設定されているか確認する
 * @param  なし
 * @retval PIN_ERROR   : 読み取りエラー
 * @retval PIN_NOT_SET : PINが設定されていない
 * @retval PIN_SET     : PINが設定されている
 * @note   マイナ免許証と通信確立された状態で実行する
 *         指定PINかDPINかどちらで照合すべきか判断するために用いる
 *         エラーをDPINと思い込んで照合すると閉塞するため、bool返却ではなくエラーを明確に分けた
 */
JPDLC_ISSET_PIN_STATUS JpDrvLicNfcCommandMynumber::issetPin(void){
    
    //PIN入っているEFがあるDFをセレクト
    JPDLC_CARD_STATUS card_status = selectInsAid();
    if(card_status == JPDLC_STATUS_ERROR){
        return PIN_ERROR;
    }

    //pinが入っているEFを短縮EF指定してREADBINARY

    std::vector<type_data_byte> retVect;

    const type_short_efid sEfid = _toShortEfid(FULL_FEID_WEF01_PINSETTING);

    retVect = parseResponseReadBinary(
            _nfcTransceive(
                assemblyCommandReadBinary_shortEFidentfy_OffsetAddr8bit(sEfid, NO_OFFSET, LE_OF_WEF01)
            )
        );

    
    //読み取りエラーだったら安全のためカードリーダーOFFできるようエラー返す(不明なまま認証走らせると閉塞する可能性がある)    
    if(retVect.empty() == true || retVect.size() != LE_OF_WEF01){
        return PIN_ERROR;
    }
    if(retVect[0] != TAG_OF_WEF01){
        return PIN_ERROR;
    }

    //PIN設定されているか
    if(retVect[2] == WEF01_PIN_SETTING_ON){
        return PIN_SET;
    }else if(retVect[2] == WEF01_PIN_SETTING_OFF){
        return PIN_NOT_SET;
    }else{
        //仕様上ありえない
        return PIN_ERROR;
    }

    return PIN_ERROR;
}

/**
 * @brief  マイナ免許証から有効期限を取得する
 * @param  なし
 * @return 成功時：有効期限 YYYY/M/D　失敗時：0年0月0日
 * @retval JPDLC_EXPIRATION_DATA 有効期限データ構造体
 * @note   マイナ免許証と通信確立され、VERIFY成功の状態で実行する
 */
JPDLC_EXPIRATION_DATA JpDrvLicNfcCommandMynumber::getExpirationData(void){

    JPDLC_EXPIRATION_DATA expirationData = {0,0,0};

    std::vector<type_data_byte> cardResVect;

    //インスタンスAIDをセレクト
    JPDLC_CARD_STATUS card_status = selectInsAid();
    if(card_status == JPDLC_STATUS_ERROR){
        return expirationData; //0000/00/00
    }
    
    //WEF02 免許情報のEF を選択
    #ifdef DLC_LAYER_DEBUG
        printf("WEF02 免許情報のEF を選択\r\n");
    #endif
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_fullEfId(FULL_FEID_WEF02_LICENSEDATA)
        )
    );
    
    if(card_status == JPDLC_STATUS_ERROR){
        return expirationData; //0000/00/00
    }

    #ifdef DLC_LAYER_DEBUG

    printf("ひとまず200文字くらい読んでみる\r\n");
    
    (void)parseResponseReadBinary(
        _nfcTransceive(
            assemblyCommandReadBinary_onlyCurrentEF_OffsetAddr15bit(0,200)
        )
    );

    #endif

    cardResVect.clear();

    cardResVect = readBinary_currentFile_specifiedTag(TAG_OF_EXPIRATION_DATA); 
    
    #ifdef DLC_LAYER_DEBUG
        printf("セキュア領域から読めた有効期限データ；");
        for (int i = 0; i < cardResVect.size(); i++)
        {
            printf("%02X ",cardResVect[i]);
        }
        printf("\n");
    #endif
    
    if(cardResVect.empty() == true){
        return expirationData;
    }

    if(cardResVect.size() > 7){
        return expirationData;
    }

    if(jisX0201toInt(cardResVect[0]) != REIWA_CODE){
        return expirationData;
    }

    uint16_t exData_reiwa = 10 * jisX0201toInt(cardResVect[1]) + jisX0201toInt(cardResVect[2]);
    expirationData.yyyy = _reiwaToYYYY(exData_reiwa);

    expirationData.m = 10 * jisX0201toInt(cardResVect[3]) + jisX0201toInt(cardResVect[4]);
    expirationData.d = 10 * jisX0201toInt(cardResVect[5]) + jisX0201toInt(cardResVect[6]);

    return expirationData;
}

/**
 * @brief  マイナ免許証のPIN残り照合回数を取得する
 * @param  なし
 * @return 成功時：残り照合回数　失敗時：0
 * @note   マイナ免許証と通信確立された状態で実行する
 *         ボディーなしの VERIFYコマンドを実行して確認する
 */
uint8_t JpDrvLicNfcCommandMynumber::getRemainingCount(void){
    
    //PIN入っているEFがあるDFをセレクト
    JPDLC_CARD_STATUS card_status = selectInsAid();

    if(card_status == JPDLC_STATUS_ERROR){
        return 0;
    }

    //pinが入っているEFを短縮EF指定してボディーなしのVerify
    uint8_t remainingCount = 0;

    remainingCount = parseResponseVerify_checkRemainingCount(
        _nfcTransceive(
            assemblyCommandVerify_checkRemainingCount(FULL_FEID_IEF01_PIN)
        )
    );

    return remainingCount;
}

/**
 * @brief  マイナ免許証のPINを照合する
 * @param  pin JIS X 0201 文字コードでコードされた PIN または DPIN
 * @retval true  : 照合成功
 * @retval false : 照合失敗
 * @note   マイナ免許証と通信確立された状態で実行する
 *         ボディーありの VERIFYコマンドを実行して確認する
 * @attention  失敗系をテストしたい場合は_nfcTransceive を _nfcTransceive_Stub に置き換える
 *             本物を閉塞させると解除してもらいに免許センターに行く必要がある
 */
bool JpDrvLicNfcCommandMynumber::executeVerify(type_PIN pin){
    
    //PIN入っているEFがあるDFをセレクト
    JPDLC_CARD_STATUS card_status = selectInsAid();

    if(card_status == JPDLC_STATUS_ERROR){
        return false;
    }

    bool retVal = false;

    retVal = parseResponseVerify_execute(
        _nfcTransceive(
            assemblyCommandVerify_execute(FULL_FEID_IEF01_PIN, pin)
        )
    );
    return retVal;
}


/* private */

JPDLC_CARD_STATUS JpDrvLicNfcCommandMynumber::selectInsAid(){
    
    //処置不要
    if(current_selected == INS){
        return JPDLC_STATUS_OK;
    }

    //インスタンスAIDをセレクト
    JPDLC_CARD_STATUS card_status = JPDLC_STATUS_ERROR;

    //AID_INS があるか
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_INS, sizeof(AID_INS)/sizeof(AID_INS[0]))
        )
    );

    if(card_status == JPDLC_STATUS_OK){
        current_selected = INS;
    }else{
        current_selected = NOT_SELECTED;
    }

    return card_status;
}
