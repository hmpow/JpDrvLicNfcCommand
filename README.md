# JpDrvLicNfcCommand
日本の運転免許証NFCコマンド Japanese Driver's License Card Nfc Command  

ICカード免許証・マイナ免許証のコマンドを生成・解釈するライブラリです。  
免許証をタッチしないとエンジン掛からないシステム( https://github.com/hmpow/DLC_Starter )
の部品として作成したものです。  

有効期限を読み出すために作成したものであり、電子署名関係は非対応です。  

## 入力仕様書
運転免許証及び運転免許証作成システム等仕様書（仕様書バージョン番号:010）を入力としています。    
「免許証 仕様書V10」でGoogle検索し、警察庁が公開している『原議保存期間 5年(令和12年3月31日まで)』pdfをダウンロードします。  

## 環境
下記で開発しています。  

マイコンボード : Arduino UNO R4 WiFi  

カードリーダ : SONY RC-S660/S  

PC : Windows 11 Pro, VSCode, PlatformIO  

カードリーダライブラリ : https://github.com/hmpow/RC_S660_S_DriverForMCU  
※サンプルとして RC_S660_S_DriverForMCU に依存していますが、"jpdlc_base_reader_if.cpp" 内の "_nfcTransceive" 関数を書き換えることで任意のカードリーダーを接続できます。  

## 構造

![image](https://hmpower.sakura.ne.jp/github_img/jpdrvlicnfccommand/about.gif)

- `jpdlc_base` は、共通ロジックのインタフェースを格納したパッケージで "jpdlc_base_" で始まる2つのcppファイルで構成されます
  - `jpdlc_base.cpp` は読み取り処理の共通部分を提供します。
  - `jpdlc_base_reader_if.cpp` はカードリーダードライバ関数をラップする層で、使用するカードリーダーに応じてユーザーが変更します。
- `reader_if` は、アプリ側で初期化～カード捕捉・アクティブ化まで完了して、NFCコマンドのやり取りができる状態に準備されたカードリーダインスタンスをポインタ経由で借用して動作します。
- アプリは`jpdlc_base` を継承して作られたクラスをインスタンス化して使用します。
  - `JpDrvLicNfcCommandMynumber` マイナ免許証
  - `JpDrvLicNfcCommandConventional` 従来免許証
