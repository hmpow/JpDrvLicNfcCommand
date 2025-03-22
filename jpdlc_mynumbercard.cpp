#include "jpdlc_mynumbercard.h"

/***************/
/* マイナ免許証 */
/***************/

const type_data_byte AID_ELF[] = { 0xA0,0x00,0x00,0x02,0x31,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
const type_data_byte AID_EXE[] = { 0xA0,0x00,0x00,0x02,0x31,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
const type_data_byte AID_INS[] = { 0xA0,0x00,0x00,0x02,0x31,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

const type_full_efid FULL_FEID_WEF01_PINSETTING   = 0x001A; //PIN設定
const type_full_efid FULL_FEID_IEF01_PIN          = 0x0006; //PIN1
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

    //AID_ELF があるか
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_ELF, sizeof(AID_ELF)/sizeof(AID_ELF[0]))
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

    //AID_ELF があるか
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_ELF, sizeof(AID_ELF)/sizeof(AID_ELF[0]))
        )
    );

    if(card_status == JPDLC_STATUS_ERROR){
        return false;
    }

    //AID_EXE があるか
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_EXE, sizeof(AID_EXE)/sizeof(AID_EXE[0]))
        )
    );

    if(card_status == JPDLC_STATUS_ERROR){
        return false;
    }

    //AID_INS があるか
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

    std::vector<type_data_byte> retVect;

    retVect = readBinary_currentFile_specifiedTag(TAG_OF_EXPIRATION_DATA); 
    if(retVect.empty() == true){
        return expirationData;
    }

    if(retVect.size() > 7){
        return expirationData;
    }

    if(retVect[0] != REIWA_CODE){
        return expirationData;
    }

    uint16_t exData_reiwa = 10 * jisX0201toInt(retVect[1]) + jisX0201toInt(retVect[2]);
    expirationData.yyyy = _reiwaToYYYY(exData_reiwa);

    expirationData.m = 10 * jisX0201toInt(retVect[3]) + jisX0201toInt(retVect[4]);
    expirationData.d = 10 * jisX0201toInt(retVect[5]) + jisX0201toInt(retVect[6]);

    return expirationData;
}

uint8_t JpDrvLicNfcCommandMynumber::getRemainingCount(void){
    
    //PIN入っているEFがあるDFをセレクト
    JPDLC_CARD_STATUS card_status = JPDLC_STATUS_ERROR;

    //AID_ELF があるか
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_ELF, sizeof(AID_ELF)/sizeof(AID_ELF[0]))
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

    //AID_ELF があるか
    card_status = parseResponseSelectFile(
        _nfcTransceive(
            assemblyCommandSelectFile_AID(AID_ELF, sizeof(AID_ELF)/sizeof(AID_ELF[0]))
        )
    );

    if(card_status == JPDLC_STATUS_ERROR){
        return false;
    }

    //pinが入っているEFを短縮EF指定してVerify
    bool retVal = false;

    retVal = parseResponseVerify_execute(
        _nfcTransceive_Stub(
            assemblyCommandVerify_execute(FULL_FEID_IEF01_PIN, pin)
        )
    );

    return retVal;
}