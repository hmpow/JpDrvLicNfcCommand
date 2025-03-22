#ifndef JP_DRV_LIC_NFC_COMMAND_BASE_H
#define JP_DRV_LIC_NFC_COMMAND_BASE_H

/* 特定のカードリーダーに依存しない　コマンドを組み立てる機能＋レスポンスを解析する機能 */
/* このクラスを書いていてカードリーダのマニュアルを見たくなったら責務配置がおかしい */

#include "jpdlc_typedef.h"
#include "jpdlc_base_reader_if.h"

#define SHOW_DEBUG 1


/*******************/
/* 従来・マイナ共通 */
/*******************/

#define DPIN         0x2A   // PIN未設定時のデフォルト値 '*' の文字コード
#define CURRENT_EF   0x0000 // 現在選択されているEFを示す
#define READ_MAX_LEN 0x0000 //ReadBinary使用時に拡張Leで読み出せる最大長 65536バイトを指示　事実上の無制限


class JpDrvLicNfcCommandBase
{

public:
    JpDrvLicNfcCommandBase();
    virtual ~JpDrvLicNfcCommandBase();


    Rcs660sAppIf rcs660sInstance;
    void setReader(const Rcs660sAppIf);


    /*********************************** 自動I/F ***********************************/

    /* 共通 */
    std::vector<type_data_byte> readBinary_currentFile_specifiedTag(type_tag); //tag指定で読んでlen分進めてってやる関数
    
    /* 種別ごと */
    virtual JPDLC_ISSET_PIN_STATUS issetPin(void)          = 0;  //PIN設定EFをSELECTしてREADBINARY
    virtual bool                   isDrvLicCard(void)      = 0;  //運転免許証かどうか
    virtual JPDLC_EXPIRATION_DATA  getExpirationData(void) = 0;  //有効期限情報を取得
    virtual uint8_t                getRemainingCount(void) = 0;  //残り試行回数を取得
    virtual bool                   executeVerify(type_PIN) = 0;  //PIN認証を実行

/* memo */
/* 子クラスだけで実装したいvirtualたちがリンクエラーになる件についてChatGPTと会話 */
/* Javaで言うabstractを実現するためには「純粋仮想関数」というのを用いると良い */
/* 書き方は virtual 付けて宣言 し、= 0 を付ける */
/* 単なる仮想関数だと、親クラス側のcppに実装がないと*/
/* いざオーバーライドしようと思った時に対象が居ないのでundefinedになる */
/* 対応としては親クラスの .cpp に 何もしない 関数を実装 */
/* しかし可読性が悪いので、純粋仮想関数を使った方が良い */


    /********************************* SELECT FILE *********************************/

    std::vector<type_data_byte>  assemblyCommandSelectFile_fullEfId(type_full_efid);
    std::vector<type_data_byte>  assemblyCommandSelectFile_AID(const type_data_byte*, const uint8_t); //AID, AID_length

    JPDLC_CARD_STATUS parseResponseSelectFile(std::vector<type_data_byte>);

    /************************************ VERIFY ************************************/
    std::vector<type_data_byte> assemblyCommandVerify_execute(type_short_efid, type_PIN);
    std::vector<type_data_byte> assemblyCommandVerify_checkRemainingCount(type_short_efid);

    bool parseResponseVerify_execute(std::vector<type_data_byte>);
    uint8_t parseResponseVerify_checkRemainingCount(std::vector<type_data_byte>);


    /********************************* READ BINARY *********************************/

    std::vector<type_data_byte> assemblyCommandReadBinary_onlyCurrentEF_OffsetAddr15bit(const uint16_t, const uint16_t); //OffsetAddr,Le
    std::vector<type_data_byte> assemblyCommandReadBinary_shortEFidentfy_OffsetAddr8bit(const type_short_efid, const uint8_t, const uint16_t); //shortEFidentfy, OffsetAddr, Le

    std::vector<type_data_byte> parseResponseReadBinary(std::vector<type_data_byte>);

    /*********************************** ツール ***********************************/

    uint8_t jisX0201toInt(type_data_byte);
    type_data_byte intTojisX0201(uint8_t);

#if 0
    char* jisX0208toShiftJis(char*);
    char* jisX0208toUtf8(char*);
#endif

protected:
//privateにするとサブクラスからアクセスできないのでダメ

    std::vector<type_data_byte> _assemblyCommandReadBinary_Base(const type_data_byte, const type_data_byte, const uint16_t);
    
    /* ツール */
    type_short_efid _toShortEfid(const type_full_efid);

    bool _isEffectiveShortEfId(const type_short_efid);

    uint16_t _reiwaToYYYY(uint16_t);
};

#endif // JP_DRV_LIC_CNF_COMMAND_BASE_H