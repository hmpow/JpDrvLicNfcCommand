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


JPDLC_ISSET_PIN_STATUS JpDrvLicNfcCommandMynumber::issetPin(void){
    
    //PIN入っているEFがあるDFをセレクト
    JPDLC_CARD_STATUS card_status = JPDLC_STATUS_ERROR;

    //AID_INS があるか
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_INS, sizeof(AID_INS)/sizeof(AID_INS[0]))
        )
    );

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

    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_INS, sizeof(AID_INS)/sizeof(AID_INS[0]))
        )
    );

    if(card_status == JPDLC_STATUS_ERROR){
        return false;
    }

    //ここまでreturn されなければOK
    return true;
}

JPDLC_EXPIRATION_DATA JpDrvLicNfcCommandMynumber::getExpirationData(void){

    JPDLC_EXPIRATION_DATA expirationData = {0,0,0};

    std::vector<type_data_byte> cardResVect;

    //インスタンスAIDをセレクト
    JPDLC_CARD_STATUS card_status = JPDLC_STATUS_ERROR;

    //マイナ免許はここに来る前に必ずVerify時にインスタンスAID一択で選択されているはずなので不要
    //仕様書流れずにも再選択は記載なし
#if 0
    //AID_INS があるか
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_INS, sizeof(AID_INS)/sizeof(AID_INS[0]))
        )
    );
    if(card_status == JPDLC_STATUS_ERROR){
        return expirationData; //0000/00/00
    }
#endif
    
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

uint8_t JpDrvLicNfcCommandMynumber::getRemainingCount(void){
    
    //PIN入っているEFがあるDFをセレクト
    JPDLC_CARD_STATUS card_status = JPDLC_STATUS_ERROR;

    //AID_INS があるか
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_INS, sizeof(AID_INS)/sizeof(AID_INS[0]))
        )
    );

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


bool JpDrvLicNfcCommandMynumber::executeVerify(type_PIN pin){
    
    //PIN入っているEFがあるDFをセレクト
    JPDLC_CARD_STATUS card_status = JPDLC_STATUS_ERROR;

    //AID_INS があるか
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_INS, sizeof(AID_INS)/sizeof(AID_INS[0]))
        )
    );

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