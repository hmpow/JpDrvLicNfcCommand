/** 
 * @file jpdlc_base_reader_if.cpp
 * @brief カードリーダーと接続する部分およびVERIFYコマンドテスト用のスタブ実装
 * @note  _nfcTransceiveを使用するカードリーダに合わせて変更
 */

#include "jpdlc_base_reader_if.h"

const uint16_t READER_TIMEOUT_MS = 200; //マイナ免許証は長めにしないとダメ

//カードリーダのインスタンスはmain.cppの持ち物なのでポインタで
Rcs660sAppIf *p_rcs660sInstance;

void setReaderInstance(Rcs660sAppIf * const p_reader){
    p_rcs660sInstance = p_reader;
    return;
}


/**
 * @brief  NFCカードにコマンドを送信し、応答を受信する
 * @param  inputCommand コマンドデータ
 * @retval retResponse 応答データ
 * @note   下記になるように使用するカードリーダに合わせて実装を変える
 *         ・std::vector<type_data_byte>型で送受信する　※type_data_byte型の実体はuint8_t型
 *         ・エラーの場合は空のvectorを返却する
 */
std::vector<type_data_byte> _nfcTransceive(const std::vector<type_data_byte> inputCommand){
    
    std::vector<type_data_byte> command;
    std::vector<type_data_byte> retResponse;

    const uint16_t comLen = inputCommand.size();

    if(comLen > READER_MAX_COMMAND_LEN){
        //カードリーダーが対応する最大長を超えている
        return retResponse; //空のvector
    }

    for(uint16_t i = 0; i < comLen; i++){
        command.push_back( (type_data_byte)inputCommand[i] );
    }

#ifdef DLC_LAYER_SHOW_COMMUNICATION_FRAME
    printf("Tx To Card : ");
    for(const auto& comByte : command){
        printf("%02X ",comByte);
    }
    printf("\r\n");
#endif

    std::vector<uint8_t> orgResponse;
    orgResponse = p_rcs660sInstance->communicateNfc(command, READER_TIMEOUT_MS);  

#ifdef DLC_LAYER_SHOW_COMMUNICATION_FRAME
    printf("Rx From Card : ");
    for(const auto& comByte : orgResponse){
        printf("%02X ",comByte);
    }
    printf("\r\n");
    printf("\r\n");
#endif

    for(const auto& orgResByte : orgResponse){
        retResponse.push_back( (type_data_byte)orgResByte );
    }

    return retResponse;
}

/**
 * @brief  VERIFYコマンドテスト用のスタブ
 * @param  inputCommand コマンドデータ
 * @retval retResponse 応答データ
 * @note   本物のカードリーダーを使用すると、間違えると閉塞するのでスタブを用意
 *         用途は下記
 *         ・カードに送信されるコマンドを画面表示し、本物で試す前に目視チェックを行う
 *         ・失敗系の応答を返却(本物を閉塞させると解除してもらいに免許センターに行く必要がある)
 */
std::vector<type_data_byte> _nfcTransceive_Stub(const std::vector<type_data_byte> inputCommand){
    
    /* VERIFYコマンドテスト用のスタブ 本物は間違えると閉塞する */

    static int stucTestCount = 0;
    std::vector<type_data_byte> empty; 
    std::vector<type_data_byte> success        = {0x90, 0x00};
    std::vector<type_data_byte> fail_infRetry  = {0x6F, 0x00};
    std::vector<type_data_byte> fail_canRetry  = {0x6F, 0xC5};
    std::vector<type_data_byte> fail_zero      = {0x6F, 0xC0};
    std::vector<type_data_byte> wrong_LcLe     = {0x67, 0x00};
    std::vector<type_data_byte> already_Locked = {0x69, 0x84};
    std::vector<type_data_byte> wrong_P1P2     = {0x6A, 0x86};

    std::vector<type_data_byte> simulateResponse[] = {
        success,
        fail_infRetry,
        fail_canRetry,
        fail_zero,
        wrong_LcLe,
        already_Locked,
        wrong_P1P2
    };

    printf("\r\n\r\n\r\n\r\n\r\nStub_nfcTransceive\r\n\r\n\r\n\r\n\r\n\r\n");
    printf("\r\n");
    std::vector<type_data_byte> command;
    std::vector<type_data_byte> retResponse;

    const uint16_t comLen = inputCommand.size();

    if(comLen > READER_MAX_COMMAND_LEN){
        //カードリーダーが対応する最大長を超えている
        return retResponse; //空のvector
    }

    for(uint16_t i = 0; i < comLen; i++){
        command.push_back( (type_data_byte)inputCommand[i] );
    }

    printf("Stub_Tx To Card : ");
    for(const auto& comByte : command){
        printf("%02X ",comByte);
    }
    printf("\r\n");

    if(stucTestCount < 7){
        retResponse = simulateResponse[stucTestCount];
        stucTestCount++;

        printf("Stub_Rx From Card : ");
        for(const auto& comByte : retResponse){
            printf("%02X ",comByte);
        }
    
    }else{
        stucTestCount = 0;
        retResponse = empty;
        printf("Stub_Rx From Card : empty Vector");
    }

    printf("\r\n");
    printf("\r\n");

    return retResponse;
}

