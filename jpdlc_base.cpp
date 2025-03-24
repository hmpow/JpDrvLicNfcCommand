#include "jpdlc_base.h"

//ToDo:リーダー内部バッファ上限で区切ってReadBinaryループ

/*******************/
/* 従来・マイナ共通 */
/*******************/

const JPDLC_STATUS_RESPONSE STATUS_SUCCESS      = {0x90, 0x00};
const JPDLC_STATUS_RESPONSE STATUS_DF_IS_LOCKED = {0x62, 0x83};
const JPDLC_STATUS_RESPONSE STATUS_VERIFY_NG    = {0x63, 0x00};

//2バイトLENの識別フラグは従来・マイナ共通
const type_data_byte TWO_BYTE_LEN_FLAG = 0x82;



JpDrvLicNfcCommandBase::JpDrvLicNfcCommandBase(){
    //コンストラクタ
}

JpDrvLicNfcCommandBase::~JpDrvLicNfcCommandBase(){
    //デストラクタ
}

/*******************************************************************************/
/*********************************** 共通I/F ***********************************/
/*******************************************************************************/

//指定タグを線形探索して読む関数
std::vector<type_data_byte> JpDrvLicNfcCommandBase::readBinary_currentFile_specifiedTag(type_tag tag){

    //現状1バイトタグのみ対応
    const type_data_byte targetTagByte = (type_data_byte)(tag & 0xFF); //下位8bit

    std::vector<type_data_byte> retVect;

    std::vector<type_data_byte> cardResVect;


    uint16_t currentOffset = 0;
    uint16_t len = 0;
    type_data_byte responseTag = 0x00;
    bool continueFlag = true;

    do{
        cardResVect.clear();
        len = 0;
        responseTag = 0x00;

        //Tag解析
        #ifdef DLC_LAYER_DEBUG
            printf("タグ探し関数 次の指示offset = %04X\n", currentOffset);
        #endif
        

        cardResVect = parseResponseReadBinary(
            _nfcTransceive(
                assemblyCommandReadBinary_onlyCurrentEF_OffsetAddr15bit(currentOffset, 2)
            )
        );

        if(cardResVect.empty()){
            //エラー
            return retVect; //空ベクター
        }

        //Tag解析
        responseTag = cardResVect[0];
        #ifdef DLC_LAYER_DEBUG
            printf("タグ探し関数 tag = %02X\n", responseTag);
        #endif
        
        //Len解析
        //Lenの頭がTWO_BYTE_LEN_FLAG(0x82)の時はLenが0x82に続く2バイト

        if(cardResVect[1] == TWO_BYTE_LEN_FLAG){
            currentOffset += 2; // tag と TWO_BYTE_LEN_FLAG 分進める
            cardResVect = parseResponseReadBinary(
                _nfcTransceive(
                    assemblyCommandReadBinary_onlyCurrentEF_OffsetAddr15bit(currentOffset, 2)
                )
            );
            if(cardResVect.empty()){
                //エラー
                return retVect; //空ベクター
            }
            //2バイトLEN
            len = (uint16_t)(cardResVect[0]) << 8 | (uint16_t)(cardResVect[1]);
        #ifdef DLC_LAYER_DEBUG
            printf("タグ探し関数 2バイトlen = %04X\n", len);
        #endif
        }else{
            //1バイトLEN
            len = (uint16_t)cardResVect[1];
        #ifdef DLC_LAYER_DEBUG
            printf("タグ探し関数 1バイトlen = %04X\n", len);
        #endif
        }

        currentOffset += 2; //tagとlen分 or 2バイトLen分進める

        if(targetTagByte == responseTag){
            //目的のタグ

            if(len == 0){
                //目的のデータのLENが0(格納されていない)ならば終了
                continueFlag = false;
                return retVect; //空ベクター
            }
          
            //目的のタグでデータがあればlen分だけ読む
            retVect = parseResponseReadBinary(
                _nfcTransceive(
                    assemblyCommandReadBinary_onlyCurrentEF_OffsetAddr15bit(currentOffset, len)
                )
            );
            continueFlag = false;
            return retVect;
        }else{
            //目的のタグでなければlen分だけ飛ばす
            currentOffset += len; //len分進める
        }
    }while(continueFlag);

    //読めたらリターンするのでここには来ないはず

    return retVect;
}


bool JpDrvLicNfcCommandBase::executeVerify_DecimalInput(type_PIN pinDecimal){
    
    type_PIN pinJisX0201 = {0,0,0,0};

    for(int i = 0; i < 4; i++){
        if(0 <= pinDecimal[i] && pinDecimal[i] <= 9){
            //0～9
            pinJisX0201[i] = intTojisX0201(pinDecimal[i]);
        }else{
            //0～9でないデータが入っていた
            return false;
        }
    }

    return executeVerify(pinJisX0201);
}

/*******************************************************************************/
/********************************* SELECT FILE *********************************/
/*******************************************************************************/

std::vector<type_data_byte> JpDrvLicNfcCommandBase::assemblyCommandSelectFile_fullEfId(type_full_efid fullEfid){
    std::vector<type_data_byte> command;
    command.push_back(0x00); //CLA
    command.push_back(0xA4); //INS
    command.push_back(0x02); //P1
    command.push_back(0x0C); //P2
    command.push_back(0x02); //Lc
    command.push_back((fullEfid >> 8) & 0xFF); //EF識別子上位8bit
    command.push_back(fullEfid & 0xFF);        //EF識別子下位8bit
    return command;
}


std::vector<type_data_byte> JpDrvLicNfcCommandBase::assemblyCommandSelectFile_AID(const type_data_byte aid[], const uint8_t aidlen){
    std::vector<type_data_byte> command;
    command.push_back(0x00); //CLA
    command.push_back(0xA4); //INS
    command.push_back(0x04); //P1
    command.push_back(0x0C); //P2
    command.push_back((type_data_byte)aidlen); //Lc
    
    for(uint8_t i = 0; i < aidlen; i++){
        command.push_back(aid[i]);             //AID
    }
    
    return command;
}



JPDLC_CARD_STATUS JpDrvLicNfcCommandBase::parseResponseSelectFile(std::vector<type_data_byte> inputVect){
    if (inputVect.size() != 2){
        return JPDLC_STATUS_ERROR;
    }
    
    if(inputVect[0] == STATUS_SUCCESS.sw1 && inputVect[1] == STATUS_SUCCESS.sw2){
        return JPDLC_STATUS_OK;
    }

    if(inputVect[0] == STATUS_DF_IS_LOCKED.sw1 && inputVect[1] == STATUS_DF_IS_LOCKED.sw2){
        return JPDLC_STATUS_WARNING;
    }

    return JPDLC_STATUS_ERROR;
}

/*******************************************************************************/
/*********************************** VERIFY ************************************/
/*******************************************************************************/

std::vector<type_data_byte> JpDrvLicNfcCommandBase::assemblyCommandVerify_execute(type_short_efid sEfid, type_PIN pin){
    std::vector<type_data_byte> command;

    //efidチェック
    if(sEfid > 0x1E){
        //短縮EFは 0b11110 まで
        return command; //空のvector
    }

    uint8_t pinLen = pin.size();
    
    //PINのチェック

    //type_PIN を使う段階で4バイト補償のためPIN長さチェックは不要

    for(uint8_t i = 0; i < pinLen; i++){
        if(pin[i] < intTojisX0201(0) || intTojisX0201(9) < pin[i]){
            //数値ではない文字
            return command; //空のvector
        }
    }

    //検査PASS

    command.push_back(0x00);   //CLA
    command.push_back(0x20);   //INS
    command.push_back(0x00);   //P1
    
    type_data_byte p2 = 0x80 | sEfid; //0b10000000 | shortEFid
    
    command.push_back(p2);     //P2
    command.push_back(pinLen); //Lc

    for(uint8_t i = 0; i < pinLen; i++){
        command.push_back(pin[i]); //PIN
    }

    return command;
}

std::vector<type_data_byte> JpDrvLicNfcCommandBase::assemblyCommandVerify_checkRemainingCount(type_short_efid sEfid){
    std::vector<type_data_byte> command;

    //efidチェック
    if(sEfid > 0x1E){
        //短縮EFは 0b11110 まで
        return command; //空のvector
    }

    //検査PASS

    command.push_back(0x00);   //CLA
    command.push_back(0x20);   //INS
    command.push_back(0x00);   //P1
  
    type_data_byte p2 = 0x80 | sEfid; //0b10000000 | shortEFid
    command.push_back(p2);     //P2

    return command;
}

bool JpDrvLicNfcCommandBase::parseResponseVerify_execute(std::vector<type_data_byte> inputVect){
    if (inputVect.size() != 2){
        return false;
    }
    
    if(inputVect[0] == STATUS_SUCCESS.sw1 && inputVect[1] == STATUS_SUCCESS.sw2){
        return true;
    }

    return false;
}

uint8_t JpDrvLicNfcCommandBase::parseResponseVerify_checkRemainingCount(std::vector<type_data_byte> inputVect){

    if (inputVect.empty() || inputVect.size() != 2){
        return 0;
    }

    if(inputVect[0] == STATUS_VERIFY_NG.sw1){
        return inputVect[1] & 0x0F; //下位4bitマスク
    }

    return 0;
}

/*******************************************************************************/
/********************************* READ BINARY *********************************/
/*******************************************************************************/

/* メモLeと拡張Leを自動切り替えるコード書くこと */
std::vector<type_data_byte> JpDrvLicNfcCommandBase::assemblyCommandReadBinary_onlyCurrentEF_OffsetAddr15bit(
    const uint16_t offset, const uint16_t le)
{
    
    if(offset > 0x7FFF){
        //オフセット指定は15bitまで
        return std::vector<type_data_byte>(); //空のvector
    }

    if(le > READER_MAX_RES_LEN){
        //カードリーダーの内部バッファが足らない
        return std::vector<type_data_byte>(); //空のvector
    }

    const type_data_byte p1 = (type_data_byte)((offset >> 8) & 0x7F); //P1　Offsetの上位7bit
    const type_data_byte p2 = (type_data_byte)(offset & 0x00FF);      //P2  Offsetの下位8bit
    
    return _assemblyCommandReadBinary_Base(p1, p2, le);
}

//shortEFidentfy, OffsetAddr, Le
std::vector<type_data_byte> JpDrvLicNfcCommandBase::assemblyCommandReadBinary_shortEFidentfy_OffsetAddr8bit(
    const type_short_efid shortEfId, const uint8_t offset, const uint16_t le)
{
    const std::vector<type_data_byte> emptyVector;

    if(_isEffectiveShortEfId(shortEfId) == false){
        return emptyVector; //空のvector
    }

    //offset値範囲チェックは、引数型がuint8_tの時点で0xFFまでなので省略可

    const type_data_byte p1 = 0x80 | shortEfId; //P1
    const type_data_byte p2 = offset; //P2

    return _assemblyCommandReadBinary_Base(p1, p2, le);
}

std::vector<type_data_byte> JpDrvLicNfcCommandBase::parseResponseReadBinary(std::vector<type_data_byte> cardRes){
    
    std::vector<type_data_byte> retVect;

    uint16_t len = cardRes.size();
      
    if(cardRes.empty() == true  || len < 2){
        return retVect; //空のベクター
    }
    
    //カードのレスポンスが”90 00"(正常終了)であるかチェック

    if(cardRes[len-2] == STATUS_SUCCESS.sw1 && cardRes[len-1] == STATUS_SUCCESS.sw2){
    
        #ifdef DLC_LAYER_DEBUG
            printf("Card Status OK! %02X %02X\r\n",cardRes[len-2],cardRes[len-1]);
        #endif
    
        for (uint16_t i = 0; i < len - 2; i++)
        {
            retVect.push_back(cardRes[i]);
        }        
    
        return retVect;

    }else{
        
        #ifdef DLC_LAYER_DEBUG
            printf("Card Status ERROR! %02X %02X\r\n",cardRes[len-2],cardRes[len-1]);
        #endif
        return retVect; //空のベクター
    }
}



std::vector<type_data_byte> JpDrvLicNfcCommandBase::_assemblyCommandReadBinary_Base(const type_data_byte p1, const type_data_byte p2, const uint16_t le){
    std::vector<type_data_byte> command;
    command.push_back(0x00); //CLA
    command.push_back(0xB0); //INS
    command.push_back(p1);   //P1
    command.push_back(p2);   //P2

    //拡張Leか判定
    bool isExLe = false;
    if(le > 0xFF){
        isExLe = true;
    }

    if(le == READ_MAX_LEN){
        //拡張Leで 00 00 00 ならば65536バイト指定

        command.push_back(0x00); //Le 拡張であることの識別
        command.push_back(0x00); //Le　上位8bit
        command.push_back(0x00); //Le　下位8bit
        return command;
    }

    if(le == 256){
        //通常Leで 00 ならば256バイト指定
        //拡張Leで 01 00 でも良いがパケット長を短くするため通常leを優先で使用
        
        command.push_back(0x00); //Le
        return command;
    }

    if(isExLe){
        command.push_back(0x00); //Le 拡張であることの識別
        command.push_back((le >> 8) & 0xFF); //Le　上位8bit
        command.push_back(le & 0xFF); //Le　下位8bit
    }else{
        command.push_back(le & 0xFF); //Le
    }
   
    return command;
}


/*******************************************************************************/
/************************************ ツール ************************************/
/*******************************************************************************/

uint8_t JpDrvLicNfcCommandBase::jisX0201toInt(type_data_byte inputJisX0201){
    return inputJisX0201 - 0x30; //0x30 = '0'
}

type_data_byte JpDrvLicNfcCommandBase::intTojisX0201(uint8_t inputInt){
    return inputInt + 0x30; //0x30 = '0'
}

#if 0
char* JpDrvLicNfcCommandBase::jisX0208toShiftJis(char*){
    
}

char* JpDrvLicNfcCommandBase::jisX0208toUtf8(char*){
        
}
#endif

type_short_efid JpDrvLicNfcCommandBase::_toShortEfid(const type_full_efid inputShortEFidentfy){
    //5bitマスク
    const uint8_t bitMask = 0x1F; //0b00011111
    return inputShortEFidentfy & bitMask;
}

bool JpDrvLicNfcCommandBase::_isEffectiveShortEfId(const type_short_efid sEfid){
    //efidチェック
    if(sEfid > 0x1E){
        //短縮EFは 0b11110 まで
        return false;
    }
    return true;
}


uint16_t JpDrvLicNfcCommandBase::_reiwaToYYYY(uint16_t reiwa){
    return reiwa + 2018;
}