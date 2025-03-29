/**
 * @file jpdlc_conventional.cpp
 * @brief 従来免許証のコマンドクラス実装
 */


#include "jpdlc_conventional.h"

/***************/
/* 従来型免許証 */
/***************/

//AID
const type_data_byte AID_DF1[] = { 0xA0,0x00,0x00,0x02,0x31,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }; //記載事項
const type_data_byte AID_DF2[] = { 0xA0,0x00,0x00,0x02,0x31,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }; //写真
const type_data_byte AID_DF3[] = { 0xA0,0x00,0x00,0x02,0x48,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }; //RFU

//MFのEFID
const type_full_efid FULL_FEID_MF_CASE3             = 0x3F00; //仕様で必ずMFに飛べるEF-ID

//MF配下のEF識別子
const type_full_efid FULL_FEID_MF_EF01_COMMONDATA   = 0x2F01; //共通データ要素
const type_full_efid FULL_FEID_MF_EF02_PINSETTING   = 0x000A; //PIN設定
const type_full_efid FULL_FEID_MF_IEF01_PIN1        = 0x0001; //PIN1
const type_full_efid FULL_FEID_MF_IEF02_PIN2        = 0x0002; //PIN2　使用しない

//DF1配下のEF識別子
const type_full_efid FULL_FEID_DF1_EF01_LICENSEDATA = 0x0001; //本籍除く記載事項
const type_full_efid FULL_FEID_DF1_EF07_SIGNATURE   = 0x0007; //電子署名 2バイトLEN読みテストに使用
/* 残りは使用しないため未実装 */

//DF2配下のEF識別子
/* 使用しないため未実装 */

//DF1配下のEF識別子
/* 使用しないため未実装 */

const uint16_t       LE_OF_EF02               = 3; //T,L,V 各1byte
const type_tag       TAG_OF_EF02              = 0x0005; //PIN設定
const type_tag       TAG_EXPIRATION_MF   = 0x0045; //有効期限情報(MF側)
const type_tag       TAG_EXPIRATION_EF01 = 0x001B; //有効期限のTAG(EF01側)
const type_tag       TAG_SIGNATURE_EF07  = 0x00B1; //電子署名のTAG(EF07) 2バイトLEN読みテストに使用

const uint8_t        NO_OFFSET                = 0x00;

const type_data_byte REIWA_CODE               = 0x05;   //免許証仕様上の令和の識別コード
const type_data_byte EF02_PIN_SETTING_ON      = 0x01;   //仕様書指定値 PIN設定ありの場合
const type_data_byte EF02_PIN_SETTING_OFF     = 0x00;   //仕様書指定値 PIN設定無しの場合

/**
 * @brief  タッチされたNFC-TypeBカードが従来免許証であるか確認する
 * @param  なし
 * @retval true  : 従来免許証である
 * @retval false : 従来免許証ではない
 * @note   カードリーダがType-Bを捕捉し、通信確立された状態で実行する
 *  　　　　従来免許証ならあるはずの3つのAIDをお試し SELECT し、成功するか確認する
 */
bool JpDrvLicNfcCommandConventional::isDrvLicCard(void){

    JPDLC_CARD_STATUS card_status = JPDLC_STATUS_ERROR;

    //AID_DF1 があるか
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_DF1, sizeof(AID_DF1)/sizeof(AID_DF1[0]))
        )
    );

    if(card_status == JPDLC_STATUS_ERROR){
        return false;
    }

    //AID_DF2 があるか
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_DF2, sizeof(AID_DF2)/sizeof(AID_DF2[0]))
        )
    );

    if(card_status == JPDLC_STATUS_ERROR){
        return false;
    }

    //AID_DF3 があるか
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_DF3, sizeof(AID_DF3)/sizeof(AID_DF3[0]))
        )
    );

    if(card_status == JPDLC_STATUS_ERROR){
        return false;
    }

    //ここまでreturn されなければOK
    return true;
}


/**
 * @brief  従来免許証に PIN 1 が設定されているか確認する
 * @param  なし
 * @retval PIN_ERROR   : 読み取りエラー
 * @retval PIN_NOT_SET : PIN 1 が設定されていない
 * @retval PIN_SET     : PIN 1 が設定されている
 * @note   従来免許証と通信確立された状態で実行する
 *         指定PINかDPINかどちらで照合すべきか判断するために用いる
 *         エラーをDPINと思い込んで照合すると閉塞するため、bool返却ではなくエラーを明確に分けた
 */
JPDLC_ISSET_PIN_STATUS JpDrvLicNfcCommandConventional::issetPin(void){
    
    //MFをセレクト
    JPDLC_CARD_STATUS card_status = JPDLC_STATUS_ERROR;
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_MF_Case3()
        )
    );
    if(card_status == JPDLC_STATUS_ERROR){
        return PIN_ERROR;
    }

    //pinが入っているEFを短縮EF指定してREADBINARY
    std::vector<type_data_byte> retVect;

    const type_short_efid sEfid = _toShortEfid(FULL_FEID_MF_EF02_PINSETTING);

    retVect = parseResponseReadBinary(
            _nfcTransceive(
                assemblyCommandReadBinary_shortEFidentfy_OffsetAddr8bit(sEfid, NO_OFFSET, LE_OF_EF02)
            )
        );

    
    //読み取りエラーだったら安全のためカードリーダーOFFできるようエラー返す(不明なまま認証走らせると閉塞する可能性がある)    
    if(retVect.empty() == true || retVect.size() != LE_OF_EF02){
        return PIN_ERROR;
    }
    if(retVect[0] != TAG_OF_EF02){
        return PIN_ERROR;
    }

    //PIN設定されているか
    if(retVect[2] == EF02_PIN_SETTING_ON){
        return PIN_SET;
    }else if(retVect[2] == EF02_PIN_SETTING_OFF){
        return PIN_NOT_SET;
    }else{
        //仕様上ありえない
        return PIN_ERROR;
    }

    return PIN_ERROR;
}


/**
 * @brief  従来免許証の共通データ要素(VERIFY不要領域)から有効期限を取得する
 * @param  なし
 * @return 成功時：有効期限 YYYY/M/D　失敗時：0年0月0日
 * @retval JPDLC_EXPIRATION_DATA 有効期限データ構造体
 * @note   従来免許証と通信確立された実行する ※VERIFY不要
 */
JPDLC_EXPIRATION_DATA JpDrvLicNfcCommandConventional::getExpirationData(void){

    //MFがセレクトされている前提

    JPDLC_EXPIRATION_DATA expirationData = {0,0,0};

    //MFをセレクト
    JPDLC_CARD_STATUS card_status = JPDLC_STATUS_ERROR;
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_MF_Case3()
        )
    );
    if(card_status == JPDLC_STATUS_ERROR){
        return expirationData; //0000/00/00
    }

    //EF01 共通データ要素 を選択

    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_fullEfId(FULL_FEID_MF_EF01_COMMONDATA)
        )
    );

    if(card_status == JPDLC_STATUS_ERROR){
        return expirationData; //0000/00/00
    }
    
    //EF01 共通データ要素 をREAD BAYNARY

    std::vector<type_data_byte> retVect;

    retVect = readBinary_currentFile_specifiedTag(TAG_EXPIRATION_MF); 
    if(retVect.empty() == true){
        return expirationData;
    }

    if(retVect.size() != 11){
        return expirationData;
    }

    expirationData.yyyy = 100 * packedBCDtoInt(retVect[7]) + packedBCDtoInt(retVect[8]);
    expirationData.m = packedBCDtoInt(retVect[9]);
    expirationData.d = packedBCDtoInt(retVect[10]);

    return expirationData;
}

/**
 * @brief  従来免許証の PIN 1 残り照合回数を取得する
 * @param  なし
 * @return 成功時：残り照合回数　失敗時：0
 * @note   従来免許証と通信確立された状態で実行する
 *         ボディーなしの VERIFYコマンドを実行して確認する
 */
uint8_t JpDrvLicNfcCommandConventional::getRemainingCount(void){
    
    //MFをセレクト
    JPDLC_CARD_STATUS card_status = JPDLC_STATUS_ERROR;
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_MF_Case3()
        )
    );
    if(card_status == JPDLC_STATUS_ERROR){
        return 0;
    }

    //pinが入っているEFを短縮EF指定してボディーなしのVerify
    uint8_t remainingCount = 0;

    remainingCount = parseResponseVerify_checkRemainingCount(
        _nfcTransceive(
            assemblyCommandVerify_checkRemainingCount(FULL_FEID_MF_IEF01_PIN1)
        )
    );

    return remainingCount;
}

/**
 * @brief  従来免許証の PIN 1 を照合する
 * @param  pin JIS X 0201 文字コードでコードされた PIN 1 または DPIN
 * @retval true  : 照合成功
 * @retval false : 照合失敗
 * @note   従来免許証と通信確立された状態で実行する
 *         ボディーありの VERIFYコマンドを実行して確認する
 * @attention  失敗系をテストしたい場合は_nfcTransceive を _nfcTransceive_Stub に置き換える
 *             本物を閉塞させると解除してもらいに免許センターに行く必要がある
 */
bool JpDrvLicNfcCommandConventional::executeVerify(type_PIN pin){
    
    //MFをセレクト
    JPDLC_CARD_STATUS card_status = JPDLC_STATUS_ERROR;
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_MF_Case3()
        )
    );
    if(card_status == JPDLC_STATUS_ERROR){
        return PIN_ERROR;
    }

    //pinが入っているEFを短縮EF指定してVerify
    bool retVal = false;

    retVal = parseResponseVerify_execute(
        _nfcTransceive(
            assemblyCommandVerify_execute(FULL_FEID_MF_IEF01_PIN1, pin)
        )
    );

    return retVal;
}



  /*********************************** 従来独自 ***********************************/


std::vector<type_data_byte> JpDrvLicNfcCommandConventional::assemblyCommandSelectFile_MF_Case3(void){
    std::vector<type_data_byte> command;
    command.push_back(0x00); //CLA
    command.push_back(0xA4); //INS
    command.push_back(0x00); //P1
    command.push_back(0x00); //P2
    command.push_back(0x02); //Lc
    command.push_back((FULL_FEID_MF_CASE3 >> 8) & 0xFF); //EF識別子上位8bit
    command.push_back(FULL_FEID_MF_CASE3 & 0xFF);        //EF識別子下位8bit
    return command;
}


uint8_t JpDrvLicNfcCommandConventional::packedBCDtoInt(type_data_byte input){
    uint8_t ten,one;
    uint8_t out = 0;
    
    ten = (input >> 4) & 0x0F;
    one = input & 0x0F;
    out = 10 * (int)ten + (int)one;
  
    return out;
  }


  /* 以下は開発・テスト用 DLC starterでは使用しない */

/**
 * @brief  従来免許証の DF1/EF01 (VERIFYが必要な領域)から有効期限を取得する
 * @param  なし
 * @return 成功時：有効期限 YYYY/M/D　失敗時：0年0月0日
 * @retval JPDLC_EXPIRATION_DATA 有効期限データ構造体
 * @note   従来免許証と通信確立され、PIN 1 VERIFY成功の状態で実行する
 */
  JPDLC_EXPIRATION_DATA JpDrvLicNfcCommandConventional::getExpirationData_from_DF1_EF01(void){

    JPDLC_EXPIRATION_DATA expirationData = {0,0,0};

    std::vector<type_data_byte> cardResVect;

    //DF01を選択
    //AID_DF1 があるか
    JPDLC_CARD_STATUS card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_DF1, sizeof(AID_DF1)/sizeof(AID_DF1[0]))
        )
    );

    if(card_status == JPDLC_STATUS_ERROR){
        return expirationData;
    }

    //EF01を選択
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_fullEfId(FULL_FEID_DF1_EF01_LICENSEDATA)
        )
    );

    if(card_status == JPDLC_STATUS_ERROR){
        return expirationData;
    }


    cardResVect = readBinary_currentFile_specifiedTag(TAG_EXPIRATION_EF01); 

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

//とりあえず長いデータ読むテスト用 用途無し
std::vector<type_data_byte> JpDrvLicNfcCommandConventional::getSignature_from_DF1_EF07(void){

    std::vector<type_data_byte> retVect;
    std::vector<type_data_byte> cardResVect;


    //DF01を選択
    //AID_DF1 があるか
    JPDLC_CARD_STATUS card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_DF1, sizeof(AID_DF1)/sizeof(AID_DF1[0]))
        )
    );

    if(card_status == JPDLC_STATUS_ERROR){
        return retVect;
    }

    //EF07を選択
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_fullEfId(FULL_FEID_DF1_EF07_SIGNATURE)
        )
    );

    if(card_status == JPDLC_STATUS_ERROR){
        return retVect;
    }

    retVect = readBinary_currentFile_specifiedTag(TAG_SIGNATURE_EF07); 

    //テスト用関数のためチェックなどは省略
    //readBinary_currentFile_specifiedTagがテスト対象
    //RC-S660/Sではバッファ超過でエラー終了するはずだが2バイトLEN判定テストが目的のためOK

    return retVect;
}