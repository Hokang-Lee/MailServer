////////////////////////////////////////////////////////////
// smtp.h Copyright K.kawakami
////////////////////////////////////////////////////////////
#ifndef _SMTP_H
#define _SMTP_H
#pragma warning(disable : 4996)

//#define  VC6 // VC6++でコンパイルする場合

//#define  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理

//#define DATA_CASH 
//#define TUNING1 // MSGがブランクになる可能性があるので廃止
//#define TUNING
//#define ENTERPRISE
//#define FOR_LGWAN // LGWAN対応の送信元自動変換機能
//#define FOR_BRIDGEGATE // ブリッジゲートＯＥＭ版

//#define READING_ASYNCHRONOUS

//#define SOCKET_FAIL_TEST // 強制テスト：テスト的にエラーを発生 ソケット無応答状態の動作調査用

//#define K_SEARCH // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver

#ifndef K_SEARCH // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
#define E_POST   // E-POST OEM 版
//#define V5
#define V4
#endif
#define V3
#define IPv6

#ifdef E_POST
#define LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
#define LDAP_OPTION_LICENSE // LDAP Optionのライセンスチェックを行なう
#define LGWAN_ON        // LGWAN機能拡張
#endif

//#define HOME_VERSION   // SSL抜き
#ifdef HOME_VERSION
 #define USER_LIMIT 5 // 5 ユーザ制限
#endif

#define RFC2822_LINE_LENGTH_LIMIT 998
#define LOG_BUFFER 2048
#define DOMAIN_AUTH_SPF    // 2006.04.10 ドメイン認証 SPF方式 追加
#define ADDED_FILTER_TAG   // 一致したメールのSubject:にタグを追加する 例 [SPAM !!]
#define ADDED_FILTER_RBL   // 本文内のURLを指定したRBLへ問い合わせるオプションを追加する 例 [SPAM !!]
#define ADDED_WILDCARD_SEARCH // 大小文字＆ワイルドカード指定でトークン
#define ADDED_ENVELOPE_UNMATCH // From:ヘッダの記載アドレスにエンベロープの送信元が含まれているか確認（メールフィルタ用）

#define DNS_QUERY
///////////////////////////
// 以下 v4.66
// UPDATE_20071204　しばらく保留
//#define UPDATE_20071204 // メッセージＩＤ採番に年月日データを付加するようにした。(Bym10id)
//#define UPDATE_20090818 // 付加表題のデータサイズを256Byteまで有効にする対策


// 以下 v4.94
/* 以下は、とりあえず不要
#define UPDATE_20160707 // 上長承認をUTF-8で処理する際、全角ハイフン「－」が内部処理で文字化けする
                        // rewritecode.datファイルをインストールフォルダへ作成し、
                        // 書式) "UTF8コード(3桁)","書換UTF8コード(3桁)"
                        // 例)   EFBC8D,E28892 '－　→ ‐
*/

//#define USE_XCHATMAIL

// 以下 v4.AN
#define UPDATE_20260603 // ログ出力の排他処理化を追加
#define UPDATE_20260605 // 詳細ログファイルのオープン失敗リトライ処理の追加
#define UPDATE_20260606 // ログの書き込み信頼性向上対策
// ログの書き込み用ファイルディスクプリタを新規で開いた後保持して
//　一定時間間隔（１時間間隔）内の書込みは実行し時間を超えた時の書き
//　込みはクローズ再オープンし、書き込み時は、MUTEXで排他する。
#define UPDATE_20260610 // 本文情報がブラックリストにヒットした場合はinlogへの記録はしない
#define UPDATE_20260610A // 1セッションで繰返し送信があるとacceptlogに結果の記録が抜ける
#define UPDATE_20260610B // 1セッションで繰返し送信があるとacceptlogに結果の記録が抜ける

// 以下 v4.AM
#define UPDATE_20251218 // SMTP認証 PLAIN方式で 先頭文字列が'\0'が含まれたユーザアカウント＆パスワード情報のBase64エンコードデータのデコード後の位置取りがずれて認証に失敗する

// 以下 v4.AL
#define UPDATE_20250411 // ピリオド付きホスト名で逆引きが未設定だとハングする可能性
#define UPDATE_20250620 // 送信元ドメイン単位での送信先制限も可能に

// 以下 v4.AK
#define UPDATE_20241022 // メールフィルタによるRBL問合せリンク長が長すぎるとハングする不具合
#define UPDATE_20241111 // 題名がMIME-Qで記載されていると上長承認題名チェックの題名長さの範囲が短くなってしまう。
#define UPDATE_20241111_ML // メーリングリストの題名挿入時にMIME指定毎に異なる文字コードが指定されていると文字化けする
#define UPDATE_20241115_ML // MLへの付加表題がMIMEエンコードで記述されていると、付加表題の付け替えに失敗する
#define UPDATE_20241116_ML // MLへの付加表題がMIMEエンコードで記述されていると、付加表題の付け替えに失敗する
#define UPDATE_20241209 // 外部アカウントでのXOAUTH2での認証方式用にユーザ別認証コード保管ファイル(oauth2.dat)を各アカウントフォルダに設定可能にした
#define UPDATE_20241209_PIPE // 外部アカウントがらの内部アドレスにパイプ設定
#define UPDATE_20241210 // acceptlogのSMTP認証の方式の記録が"LOGIN"か"CRAM-MD5"しか記録されなかった不具合
#define UPDATE_20241217 // effect.datの指定でネットマスクの判定が無効になっていた不具合
#define UPDATE_20241226 // AUTH LOGINでの応答時チャレンジ文字列の設定をレジストリから変更できるようにした。

// 以下 v4.AJ
#define UPDATE20240620 // effect.datでサブネットマスクの範囲指定の高速化と不具合修正
#define UPDATE20240621 // effect.datでサブネットマスクの旧式
#define UPDATE20240728 // サブネットマスクの範囲指定の高速化
#define UPDATE_20241003 //MIME-B/Qエンコードの指定文字が小文字だとエンコードに失敗していた不具合

// 以下 v4.AI
//#define UPDATE_20231221 // （不要 apop.datが存在し中身のデータが空白の場合にパスする）SMTP認証で条件によりユーザIDがNULLで認証が成功してしまう不具合
#define UPDATE_20240112 // MIMEパートが添付のみだと分離されない
#define UPDATE_20240113 // MIMEパート無しの添付メールだと分離されない
#define UPDATE_20240114 // MIMEパート無しの添付メールで添付分離で'Content-Type'ヘッダより先に'Content-Transfer-Encoding'ヘッダがあると2重定義してしまう不具合
#define UPDATE_20240116 // メール本体の文字エンコードがbase64の場合の添付分離時のメールに、Content-Type:ヘッダが重複してしまう不具合。
#define UPDATE_20240116A // MIMEヘッダに、Content-Type:ヘッダが重複してしまう不具合。
#define UPDATE_20240117 // 上長承認通知メールへのドメイン認証ヘッダ挿入対策
#define UPDATE_20240117A // メーリングリスト＆添付分離メールへのドメイン認証ヘッダ挿入対策
#define UPDATE_20240118 // DKIM設定箇所の選択 bit0:(受信時付加)デフォルト bit1:上長承認メールへ付加 bit2:添付分離メールへ付加
#define UPDATE_20240122 // SPF/DKIM/DMARC/ARC実行パスを半角スペースを含むロングネームでも指定可能にする対策
#define UPDATE_20240127 // メーリングリスト題名付加時メールへのドメイン認証ヘッダ挿入対策
#define UPDATE_20240129 // 承認依頼メールにDKIMサインが追加されない
#define UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
#define UPDATE_20240206 // SPF/DKIM/DMARC/ARC実行パスを半角スペースを含むロングネームでも指定可能にする対策(MailtoStaffでハングする不具合)
#define UPDATE_20240219 // 付加表題付きのＭＬでの文字化け対策
#define UPDATE_20240221 // 文字コード区切りでないデータへの対応
#define UPDATE_20240228 // 付加表題もまとめてパックするオプション
#define UPDATE_20240229 // 付加表題付きのＭＬでない場合に処理不用
#define UPDATE_20240412 // メールフィルタ指定でJIS形式のMIME-Bエンコード指定で全角と半角混在文字列の場合、全角と半角区切りで残りの検索文字列が消えてしまう。
#define UPDATE_20240420 // 暗号化通信方式のセキュリティレベルの設定
//#define UPDATE_20240502 // 送信元が無い（NULL）場合の承認依頼通知メールのFrom:ヘッダにPostmasterアカウントを疑似的に追加する
#define UPDATE_20240523 // ウイルスチェック時のログ保存先が自動生成されない不具合

// 以下 v4.AH
#define UPDATE_20231017 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
#define UPDATE_20231017A // 上長承認依頼メールの作成に失敗した場合のファイルクローズ不具合
#define UPDATE_20231025 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
#define UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
#define UPDATE_20231031 // 上長承認が内部のメーリングリスト宛だと、承認メールが作成されない不具合

// 以下 v4.AG
#define UPDATE_20230523 // OPENSSLライブラリを3.1.xに変更した。
//#define UPDATE_20230602 //不要 // 上長承認、セキュアハンドラへの通知メールに添付する元メールに特殊なバイナリコードが含まれていると正しく添付が出来ない不具合。
#define UPDATE_20230620 //[保留] 受信メールに任意のヘッダを追加するオプション
#define UPDATE_20230712 // header.datにユニーク値生成変数を追加
#define ADD_XOAUTH2 // OAUTH2での認証方式を追加
#define ADD_XOAUTH2_A // OAUTH2での認証方式を１行で指定した場合の対応
//#define ADD_XOAUTH2_LOG // OAUTH2での認証方式を追加
#define ADD_XOAUTH2_D // OAUTH2の認可アプリの登録メールアドレスで一致するもののみの許可するようにした。

// 以下 v4.AF
#define UPDATE_20220603 // Received:ヘッダに接続時のTLSバージョンとChiperの表記を追加出来るようにした。 ヘッダ編集時の変数名は(&SSLINFO 又は、&TLSINFO)
#define UPDATE_20220706 // 本文にTO:トークンに続くメールアドレスがある場合、FROMヘッダと一致するか否かの判定（EMOTET対策フィルタ）
#define UPDATE_20220728 // RFC5831(821/2821) でエンベロープFROMの書式違反の判定フラグの追加
#define UPDATE_20220728A // メールフィルタのファイルにバイナリコードが含まれていると処理がスキップしてしまう不具合
#define UPDATE_20230124 // RSETコマンドでRSTファイルを削除
#define UPDATE_20230302 // 署名鍵入りメールの添付分離を行わないようにする

// 以下 v4.AE
#define UPDATE_20220427 // リッスンしたIPアドレスのFQDNをホスト名として優先する。

// 以下 v4.AD
#define UPDATE_20211229 // 添付分離で'Content-Type'ヘッダより先に'Content-Transfer-Encoding'ヘッダがあると'Content-Transfer-Encoding'ヘッダが無視される不具合。
#define UPDATE_20211231 // 承認アドレス条件に複数のワイルドカード指定を可能にした。

// 以下 v4.AC
#define UPDATE_20211020 // 特定接続元IPからの複写転送先指定無効アドレス設定テーブル追加(ccdisableip.dat)
#define UPDATE_20211209 // IPv6判定に失敗する不具合
#define UPDATE_20211210_IPV6PREFIX // IPv6アドレスプレフィックスに対応した。
#define UPDATE_20211213 // 複数のIPv6アドレスが指定されているとサービス起動に失敗する

// 以下 v4.AB
#define UPDATE_20210427 // 本文のCharsetに合わせてリンク挿入できるようにした。
#define UPDATE_20210427A // 本文のCharsetに合わせてリンク挿入できるようにした。
#define UPDATE_20210403_20210701 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
//#define UPDATE_20210704 //メーリングリストアドレスが複数同報されている場合添付分離後のメッセージを複写後に不要な".TM$"ファイルファイルが残されないようにする。
#define UPDATE_20210706 // 上長承認フォルダに作成した"$SG"ファイルが削除されないことがあった。
#define UPDATE_20210707 // 上長承認フォルダに作成した"$SG"ファイルが削除されないことがあった。
#define UPDATE_20210714 // セキュアハンドラ処理で承認依頼メール用のファイルが作成に失敗することがある不具合。
//#define UPDATE_20210721_LOG
//#define UPDATE_20210722_LOG
#define UPDATE_20210722 // セキュアハンドラ処理でブラックリスト対象且つ添付分離機能が有効時にメール作業フォルダ内の".TMP"ファイルが削除されないことがあった。
#define UPDATE_20210725 // セキュアハンドラ処理でブラックリスト対象且つ添付分離機能が有効時かつ1メールのサイズ上限が設定されているとメール作業フォルダ内の".TMP"ファイルが削除されないことがあった。
#define UPDATE_20210726 // 1メールのサイズ上限が設定されているとMILTER有効時に接続元のアドレスによって受信メールを破棄するオプションがスキップしてしまう。

// 以下 v4.AA
#define UPDATE_20210403 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
#define UPDATE_20210405 // 添付無しのマルチパート指定のメールだと処理がロックする
#define UPDATE_20210405A // 上長承認有効で添付ファイル分離が行われると、余分なRCPファイルが残ることがある。
#define UPDATE_20210406 //メーリングリストアドレスが複数同報されている場合添付分離後のメッセージを複写する。

// 以下 v4.A9
#define UPDATE_20210303 // メーリングリストに付加表題が無いとき題名を再構築できない不具合
#define UPDATE_20210303A // メーリングリストで投稿内容保存ファイルの拡張子の指定を可能にするオプションを追加した。
#define UPDATE_20210304 // メーリングリストで投稿内容保存ファイルの原本を保存するオプションを追加した。
#define UPDATE_20210305 // メーリングリストで投稿内容保存ファイルのHTML形式のインデックスファイルの作成を禁止するオプション追加した。
#define UPDATE_20210306 // メーリングリストの添付削除でContent-TypeがTEXT/PLAINかTEXT/HTML以外のTEXTが含まれないようにした。
#define UPADTE_20210312 // リンク挿入時にMIMEで分割した。
#define UPADTE_20210312A // Content-Type:に改行されずにboundaryが含まれると正しく添付隔離に失敗した。
#define UPDATE_20210313 // MLで添付削除指定で分離保存しない設定の時添付がMIME構想が狂う
#define UPDATE_20210314 // MLで添付削除後に続く同報先がMLで添付削除指定があると、MIME構想が狂う
#define UPDATE_20210314A // 複写転送指定に、指定以外オプションを追加した。 書式:<エンベロープのFROM>,<0 or 1>|<CCアドレス> 0:エンベロープのFROM以外 1:0:エンベロープのFROMのとき(デフォルト)
#define UPDATE_20210317 // メーリングリストあてで添付削除指定の際、メールヘッダの構造が"multipart/alternative"のとき、2番目のMIME構造が添付と判定され削除されないようにした。
#define UPDATE_20210318 // 複写転送アドレスに送信者のアドレスにも送るオプションを追加。書式:<エンベロープのFROM>,<0 or 1 or 2 or 3>|<CCアドレス> 0 or 2:エンベロープのFROM以外(2:エンベロープのFROMにもCC) 1 or 3:エンベロープのFROMのとき(デフォルト)(3:エンベロープのFROMにもCC)
#define UPDATE_20210320 // リンクメッセージをattachmentからinlineに変更すした
#define UPDATE_20210321 // Content-Typeがmultipart/related;の時の処理
#define UPDATE_20210322 // 添付にTEXTやHTMLファイルがあるとメッセージが複数挿入される不具合

// 以下 v4.A8
#define UPDATE_20201117 // IPv6で接続時の受信ポートが正しく取得できていない不具合

// 以下 v4.A7
#define UPDATE_20200131 // セキュアハンドラのブラックリストで拒絶した場合のコードを追加
#define UPDATE_20200202 // 子プロセスへの引数が空白になるデータは""（ダブルクォーテーション）で括るようにした。

// 以下 v4.A6
#define UPDATE_20190426 // 利用する暗号アルゴリズムの組み合わせオプション 

// 以下 v4.A5
#define UPDATE_20190329 // ML同報時の移動状態を詳細ログに追加
#define UPDATE_20190410 // 複数ML宛のみの同報メールで処理がロックする

// 以下 v4.A4
#define UPDATE_20190207 // 独自アカウントデータベース内のパスワードの暗号化
#define UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
#define UPDATE_20190218 // IP毎の同時接続ロックアウト機能を強化（サンプル時間と拒絶時間の設定を追加）

// 以下 v4.A3
#define UPDATE_20190108 // 承認メールをHTMLメールにする機能を追加
#define UPDATE_20190109 // 承認メールをHTMLメールにしたときutf-8に変換
#define UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換

// 以下 v4.A2 //openssl1.1.1系はRelease2,Debug2でコンパイル
#define UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換えた（SSL2を廃止し、TSL1.3を使用可能にする為）

// 以下 v4.A1
//検証試験用＝不要 #define UPDATE_20181025 // 外部DLLの呼出しポインタをグローバル化
#define UPDATE_20181026 // ヒットしたブラックリストファイルの文字列位置情報を追加表示する。


// 以下 v4.A0
#define UPDATE_20190928 // サービス停止でロックする可能性があった。
#define UPDATE_20180817 // IPv6での接続時にロックアウトのカウントが出来るようにした。
#define UPDATE_20180819A // 認証セッション中にロックアウト回数に達したら強制切断する
#define UPDATE_20180819B // 認証成功時にロックアウトカウンタのリセット機能を追加
#define UPDATE_20180821 // LOGIN認証時のロックアウトのカウントが行えなかった

// 以下 v4.99
#define UPDATE_20171229 // rainloopでSMTP認証がしい敗しないようする対策 LOGINの処理
#define UPDATE_20180324 // 承認先上長の変更時に変数が初期化後再読み込みするように変更
#define UPDATE_20180515 // LADP利用時にCRAM-MD5を有効にする
#define UPDATE_20180529 // メールフィルタ機能強化オプション使用時でユニークヘッダにワイルドカード指定を行うとハングする不具合。
#define UPDATE_20180529A // Comp:1オプションで最後尾カラムのワイルドカード指定があると処理できない

// 以下 v4.98
#define UPDATE_20170917 // hostsの設定情報が優先されない
#define UPDATE_20170923 // メーリングリストに付加表題が無いとき題名を再構築しない
#define UPDATE_20170930 // 逆上長承認で承認者への送信元エンベロープが不正アドレス等で送信先に受信拒絶されないようにする対策
// // 4.98に追加
#define UPDATE_20171102 // MILTERで空白データのコマンドをスキップしないようにした
#define UPDATE_20171102A // CLAMAVMで本文の送信がスキップされないようにする対策
#define UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
#define UPDATE_20171103  // MILTERでヘッダ置換えが失敗しないようにする対策
#define UPDATE_20171105  // MILTERのHELO処理で余分なデータが出力されてしまう不具合
#define UPDATE_20171211A // 使用可能な暗号化通信方式のバージョンを規定するオプション 0x1:SSL2禁止 0x2:SSL3禁止 0x4:TSL1禁止 0x8:TSL1.1禁止 0x10:TSL1.2禁止

// 以下 v4.97
#define UPDATE_20170619 // 中間証明書の繋がりが反映されていなかった

// 以下 v4.96
#define UPDATE_20170207 // LDAP接続失敗時のリトライ処理の修正
#define UPDATE_20170309 // 長承認または、セキュアハンドラが有効時にウイルスチェックでウイルス発見された時のクライアントへ正しくない応答メッセージを送信してしまう
#define UPDATE_20170420 // MIMEエンコードの開始しか含まれていないヘッダがあるとハングする不具合 （メーリングリスト宛に"Subject: =?"でハングする）
#define UPDATE_20170512 // ヘッダ一行の文字数が受信バッファを超えている場合、空欄行が含まれてしまう不具合


// 以下 v4.95
#define UPDATE_20161223 // セキュアハンドラ用の題名登録がリンク生成時コードの指定で影響されてしまう不具合
#define UPDATE_20170112 // データ行に改行だけの場合も除去するようにする
#define UPDATE_20170125 // セキュアハンドラでのアウトバウンドメールの受信承認メッセージの修正

// 以下 v4.94
#define UPDATE_20160830 // 接続拒絶用テーブルの指定にホワイトリストの指定を追加した。"書式 <IP-Addr.><TAB>true"
#define UPDATE_20161014 // メールフィルタ条件でRBLへの重複問合せがキャッシュ設定により失敗する
#define UPDATE_20161111 // 外部からの送信元送信先制限(逆上長承認)
#define UPDATE_20161112 // 送信先制限にエンベロープの送信元と日付ヘッダ中のトークンを指定可能にした。
#define UPDATE_20161113 // 対象アドレス欄にファイル指定を可能にする機能の追加

// 以下 v4.93
#define UPDATE_20160113 // 認証時のロックアウト機能を追加
#define UPDATE_20160114 // 接続拒絶用テーブルの指定に有効時間を設定可能にした。
#define UPDATE_20160128 // 存在しないアカウントへの認証失敗ロックアウト管理用フォルダを作成
#define UPDATE_20160415 // 送信先に指定のアドレスまたはドメインが同報として含まれていないとチェック対象にする条件
#define UPDATE_20160418 // 拡張ファイルにも併記可能にする。送信先に指定のアドレスまたはドメインが同報として含まれていないとチェック対象にする条件
#define UPDATE_20160419 // アドレス条件で、承認先上長を変更

// 以下 v4.92
#define UPDATE_20151028 // セッションスレッドの作成に失敗した時の対策
#define UPDATE_20151103 // Openssl 1.0.2dに変更した
#define UPDATE_20151111 // URIBL問合せのホワイトリストテーブルを追加
#define UPDATE_20151118 // パックされている言語の末尾のホワイトスペースを除去する
#define UPDATE_20151118A // メーリングリスト投稿時にSubjectヘッダがEUC_JPでパックされていると文字化けする。
#define UPDATE_20151118B // 上長承認対象メールのSubjectヘッダがEUC_JPでパックされていると文字化けする。
#define UPDATE_20151119 // Quoted-Printableのデコードで文字が欠ける場合がある
#define UPDATE_20151122 // 特定接続元IPからの接続拒絶用テーブル追加(rejectsmtpip.dat)隠しオプション
#define UPDATE_20151126 // リッスンＩＰ毎に証明書を選択できる対策(レジストリで指定)
#define UPDATE_20151127 // リッスンＩＰ毎に証明書を選択できる対策(sslbyip.datファイルで指定)
#define ADD_IDCACHE     // Windows,AD,LDAP連携で問合せキャッシュ機能を追加。(問合せ負荷の軽減策)

// 以下 v4.91
//20160917 夏時間が正しく表示されない不具合
#define UPDATE_20150918 // 送信元エンベロープがエイリアスと一致したとき、正しく送信元エンベロープが出力できない不具合
#define UPDATE_20101015 // アカウント別のeffect.datが無いとき全体のonly指定が無視される不具合

// 以下 v4.90
#define UPDATE_20150807 // BOSSCheckで生成したSubject:ヘッダが１行結合されて出力しないようにした。

// 以下 v4.89
#define UPDATE_20150319 // エンベロープの送信元によりメール受信の許可をする場合
//#define UPDATE_20150324 // 意味なし：エンベロープの送信先をファイルへの書込みエラーが発生した場合、リトライを最大５回行う

// 以下 v4.88
#define UPDATE_20150210 // E-POST用 MAIL FILTER インターフェースへのマクロ指定を追加。
#define UPDATE_20150212 // 接続元のアドレスによって受信メールを破棄するオプション
#define UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。

// 以下 v4.87
#define MILTER_ON // E-POST用 MAIL FILTER インターフェースを追加。

// 以下 v4.86
#define UPDATE_20141210 // 承認依頼メールのmailtoリンクの文字コード指定可能にした

// 以下 v4.85
#define UPDATE_20141115 // 独自アカウント運用時で最大文字数を128byteまで拡張可能にする対策をした。

// 以下 v4.84
#define UPDATE_20141023  // URIBL問合せの効率化（メール内の重複したドメインに対する問合せを行わないようにした）
#define UPDATE_20141023A // メールアドレス単位でもGatway.datでフォワード指定可能にする対策

// 以下 v4.83
#define UPDATE_20140704 // SMTP認証方法の指定が選択されていない指定で認証が応答してしまう不具合
#define UPDATE_20140709 // SMTP認証時のユーザ名取得でバッファオーバフローでハングする可能性
#define UPDATE_20140712 // 拡張されたメールシステムステータスコードを応答に追加
#define UPDATE_20140723 // 64bit版での送信セッション中断でハングする
#define UPDATE_20140809 //転送処理時のファイル読込みでバイナリデータが含まれていてもデータが切れてしまわない対策を追加。

// 以下 v4.82
#define UPDATE_20140618 // LGWANオプションでの同報先ヘッダの置換えで同じアドレスだけが出力される不具合
#define UPDATE_20140623 // 上長承認結果ログに送信先アドレスの記録を追加した。
#define UPDATE_20140623A // 上長承認結果ログに送信元IPの記録を追加した。
#define UPDATE_20140623B // // 上長承認機能の履歴作成時の詳細さ設定フラグを追加した（送信元IPや送信先を）0:含まない(デフォルト) 1:含む

// 以下 v4.81
#define UPDATE_20131024 // EFFECT.DATへの固定IP指定時に末尾の数値以降がワイルドカード扱いされている不具合。

// 以下 v4.80
#define UPDATE_ULONGLONG // メールボックスの保管サイズ制限を64bit長で処理可能にする "計算式 (4Gbyte*Hign値)+Low値 byte"

// 以下 v4.79
#define UPDATE_20130304 // HELO/EHLOのトークンが取得されなくなった不具合
#define UPDATE_20130221 // メールフィルタのRBL問合せ時にContent-typeを指定するとでMIME形式の時の処理が正常になっていない

// 以下 v4.78
#define DOMAIN_AUTH_DKIM    // 2013.01.17 ドメイン認証 DomainkeysとDKIM方式 追加

// 以下 v4.77
#define UPDATE_20121212 // メールフィルタのRBL問合せで"Content-Type:"ヘッダの条件と組合せ可能にした。

// 以下 v4.76
#define UPDATE_20121206 // EHLOが重複した場合はRSET処理が行われる用に修正

// 以下 v4.75
#define UPDATE_20121010 // RCPT TO:処理中の初期化されないコードによりハングする

// 以下 v4.74
#define UPDATE_20121001 // DATA命令の続きに半角空白+<CRLF>の場合も命令として許可するようにした。

// 以下 v4.73
#define UPDATE_20120919 // セッションを切らずに連続して別のメール送信が発生すると、前の上長承認状態がリセットされない不具合。

// 以下 v4.72
#define UPDATE_20111125 // 題名または添付のいづれかの一致で上長承認できる機能を追加

// 以下 v4.71
#define UPDATE_20110909 // ML名+ドメイン名が53byteを超えるとハングする

// 以下 v4.70
#define MAILFROM_MATCH_CC   // エンベロープのFROMが一致したメールをCCする。 2011.07.19追加

// 以下 v4.69
#define UPDATE_20110525 // IPv6でのIP単位での接続設定のとき内部ドメイン宛の接続を拒否してしまう不具合

// 以下 v4.68
#define UPDATE_20110119 // メーリングリストの添付削除でHTMLメールに添付があると正しく削除されない

// 以下 v4.67
#define UPDATE_20110119 // メーリングリストの添付削除でHTMLメールに添付があると正しく削除されない
#define UPDATE_201012140 // 外部DISKの場合承認のため保留通知の移動に失敗してロックする
#define UPDATE_20100902 // メール受信サイズ制限有効時に共有DISKを作業フォルダにすると通信がロックする不具合

// 以下 v4.66
#define SHORT_SURBL // 短縮URLのSURBL判定対策

// 以下 v4.65
#define UPDATE_20100129 // メールヘッダの区切りにホワイトスペースが無いとヘッダを認識しない

// 以下 v4.64
#define UPDATE_20091210 // メールフィルタのSubject等でUTF-8でのMIMEエンコードの指定でハングする
//#define UPDATE_200691215 // エイリアスで送信者信頼度のFromヘッダ一致時も可能にする
#define UPDATE_20091224 // 前バージョン環境変数%systemroot%の対策ロジックのミス

///////////////////////////
// 以下 v4.63
#define UPDATE_20090903 // メール作業領域で環境変数%systemroot%があるとドライブチェックが正しく出来ない
#define UPDATE_20090908 // ウイルスチェック用メモリの初期化 スキャンDLLロード失敗でウイルス情報として初期化されないデータが表示される可能性
#define UPDATE_20090910 // report-type="disposition-notification" なら添付無しと判定する
#define UPDATE_20091014 // 深い階層のアカウントが読めるように対策
#define UPDATE_20091014A // ドメイン無しアカウントでホームディレクトリ参照時ハングする可能性がある
#define UPDATE_20091014B // LDAPでHOMEディレクトリ参照時に問合せアカウントがNULLの場合、リトライがかかり応答に時間がかかる不具合
#define UPDATE_20091014C // LDAPへのアカウント問合せ処理の高速化
#define UPDATE_20091014D // LDAPへのアカウント認証時、ドメインに含まれないアカウントの認証も成功してしまう不具合
#define UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
#define UPDATE_20091101 // HOMEディレクトリタイプで無いならホームフォルダの検索をしないようにしAD連携時の処理速度の向上を図った。
#define UPDATE_20091103 // LDAP連携専用のリトライ回数指定の変数に変更
#define UPDATE_20091120 // ML保存時のindex.html作成のための排他処理、64bit長のファイルサイズに対応
// 承認否承認の一括処理は検討中（複数メールがうまく作成できない）
//#define UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
//#define UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
#define UPDATE_20091203A // 承認、否承認のリクエストにワイルドカード指定があると対象のセッションがロックする。

///////////////////////////
// 以下 v4.62
//#define UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更(.ADRファイルに上長アドレス保管し制御(Max:1024Byte))
#define UPDATE_20090610 // %HOME%が設定されているとまともに動かない
#define UPDATE_20090625 // IMAP4使用対策（ユーザーボックス内のサブフォルダのファイルサイズチェック）
#define UPDATE_20090701 // 代理承認処理メールの挿入文の書換えを可能に
#ifdef USE_SSL
#define USE_STARTTLS // STARTTLSコマンドを実装
#endif

#ifndef USE_SSL
#undef UPDATE_20171211A
#undef UPDATE_20190426
#undef UPDATE_20240420
#undef UPDATE_20220603
#endif
//////////////////////////
// 以下 v4.61
#define UPDATE_20090306 // TEMPフォルダのデータ残骸起動時クリア
//#define UPDATE_20090227 // 不要な対策：TEMPフォルダ内データ*.RCB,*.MSBが削除されない。backupフォルダに大量のメールが溜まり移動に時間がかかっているときにサービスを止めた場合の残骸だった。
//#define UPDATE_20090225 // 不要な対策：データ受信中プログラムのCPU使用率を下げる対策
#define UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
#define UPDATE_20090205 // バックアップメッセージの拡張子を指定するオプション
///////////////////////////
// 以下 v4.60
#define UPDATE_20090120A // 承認保留一覧取得時に承認者が３人以上の場合の一覧が取得できない
#define UPDATE_20090120 // 複数承認者のエンベロープ情報に改行抜け
#define UPDATE_20090114 //BossCheck 承認者数の設定
//#define UPDATE_20090113 // SSL終了時の処理修正（SSL_shutdown抜け）
#define UPDATE_20090108 // SSL用中間証明書
///////////////////////////
// 以下 v4.59
#define UPDATE_20081111 // LGWANオプション関連でのネットマスクのチェック範囲に誤りがあった
#define UPDATE_20080929A // ログのクリティカルセクション化
#define UPDATE_20080929 // メールフィルタ未定義の時に取得メモリ領域の開放抜け
#define UPDATE_20080916 // メーリングリスト宛の題名が長いメールでループする
///////////////////////////
// 以下 v4.58
#define UPDATE_20080903 // １メール送信ごとにセッションを切らずに連続送信すると２通目以降は片方の承認者のみにしか送信できなくなる不具合
#define UPDATE_20080812 // 送信先と同報１つ目の情報の取得をエンベロープファイルからにした
#define UPDATE_20080711 // 承認したことを、他の上長にも通知
#define UPDATE_20080710 // TOアドレスのホワイトスペース除去
#define UPDATE_20080703 // メールフィルタのRBL項目の参照DNS指定で、キャッシュ有効無効のオプションを追加
#define UPDATE_20080630 // In-Reply-Toへの対応
#define UPDATE_20080629 // 承認者は２人までに制限(approvalフォルダに保管するファイル名が、ファイル名の長さの限界があるため)
#define UPDATE_20080628 // メールフィルタのヘッダ挿入機能で、':'の区切りがない場合強制的に挿入する
#define UPDATE_20080627 // 題名付加で承認否認メールの作成でハングする可能性
#define UPDATE_20080620 // TOアドレスが代理人になる機能追加
#define UPDATE_20080617 // 上長承認機能で添付ファイルの拡張子による承認条件追加
#define UPDATE_20080616 // 上長承認メールで添付ファイルとして拡張子がHTMLのファイルを添付された場合にスルーしてしまう対策
///////////////////////////
// 以下 v4.57
#define UPDATE_20080531 // LGWAN使用時の振分け機能の強化
#define UPDATE_20080530 // 上長承認で複数上長宛の指定が一部宛先がロスとしてしまう
#define UPDATE_20080519 // 上長宛承認メールの上長アドレスが完全一致したものを抽出する
#define UPDATE_20080517 // 上長宛承認メールの承認否認メールに対象メールの題名情報などを記録する
#define UPDATE_20080516 // 複数承認者中に代理承認者設定があった場合の対策
#define UPDATE_20080515 // "waitlist_"コマンドで送信先アドレスを表示
#define UPDATE_20080514 // 上長宛承認メールのmailto:で記載した返信先アドレスのドメインを強制変更する設定
#define UPDATE_20080513 // boundary項に書式違反があるとハングする
#define UPDATE_20080512 // 同報送信時に受領した最後のアドレスを"&RECIPIENT"で表示する
#define UPDATE_20080509 // 送信先アドレスを表示
#define UPDATE_20080507 // 添付付きの判定方法の改善
#define UPDATE_20080501 // 上長承認機能でSubjectヘッダの次にContent-Typeヘッダが続くと添付の判定に失敗する
#define UPDATE_20080501 // 上長承認 uuencodeでの添付ファイル有無を認識する対策
#define UPDATE_20080423 // 対象ヘッダが複数行にまたがる#blank処理の判定
#define UPDATE_20080413 // メールフィルタの機能強化オプションを追加
#define UPDATE_20080326 // ドメイン無しアカウントをローカルとしての利用有無選択機能追加

////////////////////////////////////
#ifdef UPDATE_20080413 // メールフィルタの機能強化オプションを追加
 //　　Level:<n>
 //　　　　n=4xx ヘッダとして"Tag:"で指定した文字列の挿入を行い通過させる。
 #define ADDED_FILTER_HEADER  // 一致したメールのヘッダとして追加する。
 //　　　　n=5xx "Forward:"で指定したアドレスへ転送する。
 //Forward:<forward address>(強制転送するメールアドレスを指定します。）
 #define ADDED_FILTER_FORWARD // 一致したメールを転送する。
 //Unique-Header:<unique header>:<token>(<unique header>ヘッダの特徴を定義、項目なしもしくは空白が特徴の場合'#blank'を指定）
 #define ADDED_FILTER_UNIQUE   // Unique-Header:<unique header>:<token>(<unique header>ヘッダの特徴を定義、項目なしもしくは空白が特徴の場合'#blank'を指定）
 //　　　　n=6xx Subject:へ"Tag:"で指定した文字列の挿入を行い、"Forward:"で指定したアドレスへ転送する。
 #define ADDED_FILTER_SUBJECT_AND_FORWARD   // Subject:へ"Tag:"で指定した文字列の挿入を行い、"Forward:"で指定したアドレスへ転送する。
 //　　　　n=7xx ヘッダとして"Tag:"で指定した文字列の挿入を行い"Forward:"で指定したアドレスへ転送する。
 #define ADDED_FILTER_HEADER_AND_FORWARD   // ヘッダとして"Tag:"で指定した文字列の挿入を行い"Forward:"で指定したアドレスへ転送する。
#endif
////////////////////////////////////

///////////////////////////
// 以下 v4.56
#define UPDATE_20080317 // メッセージＩＤ用変数が初期化されない不具合
///////////////////////////
// 以下 v4.55
#define UPDATE_20080229 // 承認保留一覧に各メールの題名を表示する
#define UPDATE_20080226 // 部下に対する承認のため保留中のリスト
#define UPDATE_20080225 // 部下に対する承認のため保留通知
#define UPDATE_20080221 // 承認者の代理人の代理人の深さの設定
#define UPDATE_20080220 // 承認者の代理人の代理人を設定する
#define UPDATE_20080219 // メーリングリストの保存で付加表題がNULLの場合題名が記載されない
///////////////////////////
// 以下 v4.54
#define UPDATE_20080218 // 分割メールでの添付判定
#define UPDATE_20080204A // 題名がUNICODE utf-7/utf-8でのBossCheckで承認依頼メールの題名が文字化けする
#define UPDATE_20080204 // 題名がUNICODE utf-7/utf-8でのメーリングリスト投稿で文字化けする
#define UPDATE_20071227 // RSETコマンドでRCPファイルを削除
#define UPDATE_20071220A // 承認処理済で削除がされないことがある。
#define UPDATE_20071217 // norelayオプションでSMTP認証無視オプションの追加、"norelay,2"
#define UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
#define UPDATE_20071210 // エンベロープが255Byte(RFC2821規定)を超えた場合、拒否する。
#define UPDATE_20071209B // VRFY,EXPN命令の非応答設定での応答コードを252に変更。(RFC2821に準拠)
#define UPDATE_20071209A // VRFY命令でユーザ応答の形式の修正。(RFC2821に準拠)
#define UPDATE_20071209 // EXPN命令でメーリングリストメンバが正しく応答されないことがある不具合。
#define UPDATE_20071205 // RFC2821: 外部メールアドレス長が256Byteのときの対策
#define UPDATE_20071205A // メールアドレス長がバッファ長以上のときの対処
/////////////////////////////////////////////////////////////////////
// RFC2821によると、
// local-part : The maximum total length of a user name or other local-part is 64 characters.
// domain : The maximum total length of a domain name or number is 255 characters.
// 
// RFC821によると、
// user : The maximum total length of a user name is 64 characters.
// domain : The maximum total length of a domain name or number is 64 characters.
/////////////////////////////////////////////////////////////////////
// 以下 v4.53
#ifdef E_POST //K_SEARCH OEM 版 では使用不可
#define UPDATE_20070425 // MSCSのスタンバイ側に対応 !!有効になっていなかった!!
#endif
#define UPDATE_20071129 // 確認名称の先頭がピリオド若しくは連続するピリオドが含まれる対象はスキップする
#define UPDATE_20071121 //IPv6で全てのIPアドレスに応答のとき停止が出来ない
// 以下 v4.52
//#define UPDATE_20071113 // 属性が隠しファイルでも処理を可能にする
//#define UPDATE_20071024 // 不要 メールフィルタ処理中にCPU100%にならない対策
#define UPDATE_20071022B // メーリングリストの付加ヘッダ指定がないとループする
#define UPDATE_20071022A // 変数の初期値が不定のためにハングする可能性
#define UPDATE_20071022 // フィルタ処理などのIP比較で、完全一致しないIPにも一致と検出してしまった
#define UPDATE_20071016 // LDAPサーバーへの接続リトライ
// 以下 v4.51
#define UPDATE_20070928 // メーリングリスト MIME-Qへの対応と付加へ空なら処理しない
#define UPDATE_20070927 // メーリングリストの付加ヘッダに連番指定がないとハングする
#define UPDATE_20070922 // Comp:1オプションで先頭カラムと最後尾カラムのワイルドカード指定があると処理できない
#define UPDATE_20070914 // URIの追加判定条件 末尾に'='がある場合のみ改行を外す。
#define UPDATE_20070910 // URIの追加判定条件 //評価中
#define UPDATE_20070205 // URIのデータ正規化処理の追加 //評価中
#define UPDATE_20070730 // HTMLメールでの処理に対応
// 以下 v4.50
#define UPDATE_20070713 // 承認者の代理人を設定するオプション
#define UPDATE_20070620 // ドメインごとにDNを設定可能にする
// 以下 v4.49
#define UPDATE_20070616 // キーワードで、承認先上長を変更
#define UPDATE_20060615 // LDAPサーバーへの接続リトライ
#define UPDATE_20070614 // 上長承認で最初のRCPT TO:の条件設定を優先
#define UPDATE_20070613 // 上長承認機能追加 BXXXXXXXXX.MS$ファイル削除
#define UPDATE_20060711 // 全体の送信先制限も可能に
#define UPDATE_20070610 // 拒否指定オプションの追加
#define UPDATE_20070607A // "拡張子が".~SG"のファイルがTEMPフォルダに削除されずに残されることがある
#define UPDATE_20070607 // 上長承認機能にSubject:キーワードも可能にする
//#define UPDATE_20070607 // ドメイン部のワイルドカードを許可
#define UPDATE_20070525 // SMTP認証が成功している場合で存在しない内部ユーザは拒否するようにする
#define UPDATE_20070521 // OSの予約語対策
#define UPDATE_20070516 // 上長承認機能の追加 (メーリングリスト宛がまだ、うまく処理されない)
#define UPDATE_20070511 // IPアドレス確認ロジックの共有化
#define UPDATE_20070510 // 実ユーザ名でのメール送受信を禁止処理する処理
#define UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
/////#define UPDATE_20060507 // メールフィルタに禁止する同報の組合せ機能を追加
#define UPDATE_20060506 // メールフィルタで同じTagをを重複追加しないようにする。
// 以下 v4.46 
#ifdef E_POST //K_SEARCH OEM 版 では使用不可
#define UPDATE_20070425 // MSCSのスタンバイ側に対応
#endif
#define UPDATE_20070423 // "拡張子が".~SG"のファイルがTEMPフォルダに削除されずに残されることがある
#define UPDATE_20070407 // リンク先のドメイン名の調査する深さを調整可能に
#define UPDATE_20070405 // イベントログデータベースを追加。
#define UPDATE_20070316 // メールバックアップ先が異なるボリュームの場合移動できない不具合
#define UPDATE_20070301 // 対象アドレスの比較 *ワイルドカード有効にする 2007.03.01
#define UPDATE_20070204 // TEMPフォルダ内".MSG"のファイルが読み込み可能か確認
#define UPDATE_20070202 // 高付加時に、受信ファイルが作成完了待ちでファイルを開けずに処理がループする。（pContext->Datafpのオープン失敗を回復）
#define UPDATE_20070105 // TEMPフォルダ内のMSGファイルの削除に失敗してファイルが残ってしまうことがある
#define UPDATE_20061225 // "拡張子が".~SG"のファイルがTEMPフォルダに削除されずに残されることがある
#define UPDATE_20061218 // AUTH PLAINの初めの認可識別子を読み飛ばすように修正
#define UPDATE_20061213 // "拡張子が".~SG"のファイルがTEMPフォルダに削除されずに残されることがある
#define UPDATE_20061118 // MAIL FROM:で特殊なキャラクタを使用すると処理中のセッションの応答が出来なくなる不具合。
#define UPDATE_20061102 // 受信メールのエンベロープと本文の任意のフォルダへのバックアップ機能の追加
//#define UPDATE_20061031 // 開放されていないメモリをリセットしてしまう可能性
#define UPDATE_20061011 // ソケット処理のselect()がエラーとして返えった後、以降のソケットのリッスンが出来なくなる可能性の対策。
#define UPDATE_20061002 // ドメイン名の抽出が出来ないURIBL処理の対策
#define UPDATE_20060721 // 共有DISK方式で異なるドライブ間のファイルの移動が失敗する。
#define UPDATE_20060718 // エイリアス設定で割り当てられたエイリアスアカウントと実アカウントのドメインが異なる場合は、RCPT TO:を実アカウントに変換する。
#define UPDATE_20060710 // ワイルドカードの処理強化
#define UPDATE_20060704 // ローカルマシンのWINDOWSアカウント参照でエラーになる
#define UPDATE_20060627A // ML宛てで同報があるとinlogに記録できない
#define UPDATE_20060627 // メーリングリストアドレス判断のリセット
#define UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
#define UPDATE_20060617 // エイリアスで送信者信頼度も可能にする
#define UPDATE_20060616 // エイリアスの場合実アカウントに変換されてしまう。
#define UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
#define UPDATE_20060606 // ドメインコントローラへの接続リトライ
#define UPDATE_20060605 // SSL_acceptの応答コードが０の時に正常としていた不具合
#define UPDATE_20060515 // SOCKET関連のメモリ開放抜け
#define UPDATE_20060514 // 無駄なI/Oポートの関連付けを処理しない
#define UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
#define UPDATE_20060509 // 送信元エンベロープでのSPFのスキップチェックできるようオプションとして追加
#define UPDATE_20060329 // SSLの初期化に失敗すると以降でハングする
#define UPDATE_20060324 // Thunderbirdで添付を含むSSL送信で問題をおこす。
#define UPDATE_20060322 // SSLの初期化に失敗すると以降でハングする
#define UPDATE_20060315 // 隠すIPアドレス
#define UPDATE_20060306 // ヘッダ区切りのない本文への対応
#define UPDATE_20060116 // SMTP認証でのバッファオーバーフロー対策
#define UPDATE_20060110 // 作成ファイルがクローズしてもクローズされないのでフラッシュしてみる
//#define UPDATE_20060106 // 作成完了しているがファイル記録が削除されないのタイミングで移動待ちにならないための処理
//#define UPDATE_20051122 // うまく行かない？メッセージＩＤが更新されないタイミングの対策
#define UPDATE_20051121 // メール連続受領に対する配送キュー調整
#define FIRST_TIME_MAIL // 繰り返す同一内容のメール除去
#define UPDATE_20051108 // RCP,MSGの移動ロストしないように待つ
#define UPDATE_20051104 // RCP,MSGの移動ロストしないように待つ
//#define UPDATE_20051101 // RCP,MSGの移動ロストしないように待つ
#define UPDATE_20051027 // RCPの再作成でロストしないように待つ(自マシンドライブをUNC接続で行った場合の対策)
#define UPDATE_20051018 // RCPの再作成でロストしないように待つ
#define UPDATE_20050921 // DATA受信中に強制切断されると　354 Data を再度送出してしまう不具合
#define UPDATE_20050916 // SMTP AUTH のユーザＩＤ情報の格納
#define UPDATE_20050914  // base64メールの解凍
#define UPDATE_20050908 // １行内に入れ子を含んだ複数の条件指定があると無視されることがある
#define UPDATE_200500902 // ドメインが空欄だと、メモリが初期化されないままドメインとなる
#define UPDATE_20050819  // メールフィルタ指定ヘッダがない場合の#blankの処理
#define UPDATE_20050606  // 過負荷時にRCPファイルの再作成がロストしないようにする対策。
#define UPDATE_20050528_1// メールフィルタデータ取得メモリエラー時の対策。
#define UPDATE_20050528  // メールフィルタのFrom:指定にMIME-Qエンコード指定を可能にする。
#define UPDATE_20050518  // SSLデータ読み込み時のタイムアウトチェックの処理修正
//#define SPEED_TEST     // DATA到達で未チェック時の完了速度
#define UPDATE_20050516  // メールフィルタのSubject:指定にMIME-Qエンコード指定を可能にする。
#define UPDATE_20050512  // ドメイン無しローカルアカウントについて内部ドメイン関連付け
#define UPDATE_20050329  // 3rd-party check Pass list
#define UPDATE_20050216  // Header.datで行内に'}'が見つからない場合の対策
#define UPDATE_20050214  // Message-IDヘッダの"@マーク以降を保管してreceived:ヘッダと比較
#define UPDATE_20050111  // メーリングリスト宛のSubjectがバッファオーバーフローする可能性の対策２
#define UPDATE_20050107  // メーリングリスト宛のSubjectがバッファオーバーフローする可能性の対策１
//#define UPDATE_20041225_SPEEDUP    // 意味無し：高速化処理(正常なら前倒して応答する)
//#define UPDATE_20041224_FILESPEED  // 意味無し：TEMPでのMSGファイル操作をINCOMING内で処理するように変更＝高速化処理ほとんど変わらず。
#define UPDATE_20041208
#define UPDATE_20041204
#define UPDATE_20040721
//#define UPDATE_20040720_LOG
//#define UPDATE_20040707  保存先がおかしくなる
#define EFFECT_DAT_PLUS1
#define UPADTE_20040518
#define UPADTE_20040428
#define UPDATE_20040318
#define UPADTE_20040107
#define UPADTE_20031120
#define UPDATE_20040202
#define Y2038_BUG 1

#define WAIT_TIME 0 

#ifdef E_POST
  #define REVERS_DNS_FAILED // 逆引きで応答がないものは拒否 (v4.22)
  #define REGTOFILE
  #define CLUSTERING
  #define FILTER_PASS
  //#define FREE_LICENCE // フリーライセンス無効
  #define UPDATE_20051004 // SMTP認証ID比較でFROM:が2行以上にまたがる対策
  #define UPDATE_20051002 // WSP(ヘッダーのホワイトスペース）除去
  #define UPDATE_20050927 // SMTP AUTH のユーザＩＤ情報の格納 個人別対応
  #define UPDATE_20050924  // 送信元ユーザ別送信先制限 セキュリティ強化バージョン用
#endif

#ifdef V4
  //#define REGTOFILE
  //#define CLUSTERING
  #define THIRDPARTY
  #define THIRDPARTYMODULE   "ThirdPartyModeule"
  #define BEGINLOW  "V4BeginLow"
  #define BEGINHIGH "V4BeginHigh"
  #define LIMITKEY  "V4LimitKey"
#else
#ifdef V3
  #define THIRDPARTY
  #define THIRDPARTYMODULE   "ThirdPartyModeule"
#ifdef HOME_VERSION
  #define BEGINLOW  "V3@HomeBeginLow"
  #define BEGINHIGH "V3@HomeBeginHigh"
  #define LIMITKEY  "V3@HomeLimitKey"
#else
#ifdef K_SEARCH // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
  #define BEGINLOW  "BeginLow"
  #define BEGINHIGH "BeginHigh"
  #define LIMITKEY  "LimitKey"
#else
  #define BEGINLOW  "V3BeginLow"
  #define BEGINHIGH "V3BeginHigh"
  #define LIMITKEY  "V3LimitKey"
#endif // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
#endif
#else
  #define BEGINLOW  "BeginLow"
  #define BEGINHIGH "BeginHigh"
  #define LIMITKEY  "LimitKey"
#endif
#endif

#ifdef IPv6
#include <winsock2.h>
#ifdef VC6
#include "..\ipv6-src\inc\ws2ip6.h"
#else
#include <ws2tcpip.h>
#ifndef AI_V4MAPPED
#define AI_V4MAPPED     1
#endif
#ifndef AI_ALL
#define AI_ALL          2
#endif
#ifndef AI_ADDRCONFIG
#define AI_ADDRCONFIG   4
#endif
#ifndef AI_DEFAULT
#define AI_DEFAULT      (AI_V4MAPPED | AI_ADDRCONFIG)
#endif
const char * WSAAPI
inet_ntop(
    int AddressFamily,     // Address family to which the address belongs.
    const void *Address,   // Address (binary) to convert.
    char *OutputString,    // Where to return the output string.
    int OutputBufferSize);

int WSAAPI
inet6_addr(
    const char *InputString,   // IPv6 address (in "colon" representation).
    struct in6_addr *Address);  // Where to return binary representation.

int WSAAPI
inet_pton(
    int AddressFamily,        // Address family to which the address belongs.
    const char *InputString,  // Address (numeric string) to convert.
    void *Address);            // Where to return the binary address.
#endif
#endif


#include <windows.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <tchar.h>
#include <locale.h>
#include <memory.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <process.h>

#ifdef USE_SSL
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#define ESMTP_AUTH
#include "smtptype.h"
#include "profile.h"


//#define TRACE_MODE 1
//#define TRACE_LOG 1
#define TMQ_ON 1
//#define ACCEPT_LOG_LEVEL2 1
#define ACCEPT_LOG_LEVEL3 1
//#define TEST_MODE
#define TZNAME_MAX 50
#define MAX_FILE 50
#define NEW_PART 1
#define SLEEP_TIME 10
#define SLEEP_TIME_CLOSE   SLEEP_TIME //10
#define SLEEP_TIME_VIEW    SLEEP_TIME //20
#define SLEEP_TIME_ADD     SLEEP_TIME //30
#define SLEEP_TIME_DELETE  SLEEP_TIME //40
#define SLEEP_TIME_UPDATE  SLEEP_TIME //50
#define SLEEP_TIME_CHECK   SLEEP_TIME //60
#define SLEEP_TIME_TIMEOUT SLEEP_TIME //100

#ifdef E_POST
#ifdef FOR_BRIDGEGATE
#define ESOFTNAME       "ESMTP Receiver for BridgeGate" 
#else
#define ESOFTNAME       "ESMTP Receiver" 
#endif
#else
#ifdef V4
#define ESOFTNAME       "ESMTP Receiver @Solomon" 
#else
#ifdef HOME_VERSION
#define ESOFTNAME       "ESMTP Receiver @Home" 
#else
#ifdef K_SEARCH // K_SEARCH OEM 版 //KTEC ESMTP Archive Receiver
#define ESOFTNAME       "ESMTP Archive Receiver" 
#else
#define ESOFTNAME       "ESMTP Receiver" 
#endif
#endif
#endif
#endif

#define ADCACHE         "adrs"
#define SOFTNAME        "SMTP Receiver"
#define BOSSCHECK_REG   "SOFTWARE\\EPOST\\BOSSCHECK"
#define LDAPOPTION_REG  "SOFTWARE\\EPOST\\LDAPOPTION"
#define LDAPOPTION_BEGINLOW  "BeginLow"
#define LDAPOPTION_BEGINHIGH "BeginHigh"
#define LDAPOPTION_LIMITKEY  "LimitKey"

#define LDAP_
#ifdef E_POST
  //#define ANNOUNCE 1
  //#define MAX_QUEUE  1000
  #define TRADEMARK         "E-POST " //"Premix "
  #define SMTP_SERVICE      "EPSTRS"
  #define SMTP_SERVICE_DS   "EPSTDS"
  #define SOFT_REG          "SOFTWARE\\EMWAC\\IMS"
  #define SOFT_ALIASES_REG  "SOFTWARE\\EMWAC\\IMS\\Aliases" //"SOFTWARE\\EPOST\\IMS\\Aliases"
  #define SOFT_LISTS_REG    "SOFTWARE\\EMWAC\\IMS\\Lists"   //"SOFTWARE\\EPOST\\IMS\\Lists"
  #define DOMAIN_REG        "SOFTWARE\\EPOST\\IMS\\Domain"
  #define DOMAIN_SMTPIP     "SOFTWARE\\EPOST\\IMS\\Domain\\SmtpIP"
  #define DOMAIN_POP3IP     "SOFTWARE\\EPOST\\IMS\\Domain\\Pop3IP"
  #define DOMAIN_FOLDER     "SOFTWARE\\EPOST\\IMS\\Domain\\Folder"
  #define DOMAIN_BASEDN     "SOFTWARE\\EPOST\\IMS\\Domain\\BDN"
  #define MAIL_SPOOL        "%SystemRoot%\\SYSTEM32\\E-POST\\MAIL"
#else
  //#define ANNOUNCE 1
  //#define MAX_QUEUE  10
#ifdef V3
#ifdef K_SEARCH // K_SEARCH OEM 版
  #define TRADEMARK         "KTEC "
  #define SMTP_SERVICE      "KSARS"
  #define SMTP_SERVICE_DS   "KSADS"
#else
  #define TRADEMARK         "SPA-PRO "
  #define SMTP_SERVICE      "SPARS-PRO"
  #define SMTP_SERVICE_DS   "SPADS-PRO"
#endif
#else
  #define TRADEMARK         "SPA "
  #define SMTP_SERVICE      "SPARS"
  #define SMTP_SERVICE_DS   "SPADS"
#endif
#ifdef K_SEARCH // K_SEARCH OEM 版
  #define SOFT_REG          "SOFTWARE\\KTEC\\KSARS"
  #define SOFT_ALIASES_REG  "SOFTWARE\\KTEC\\KSARS\\Aliases"
  #define SOFT_LISTS_REG    "SOFTWARE\\KTEC\\KSARS\\Lists"
  #define DOMAIN_BASEDN     "SOFTWARE\\KTEC\\KSARS\\Domain\\BDN"
#else
  #define SOFT_REG          "SOFTWARE\\EMWAC\\IMS"
  #define SOFT_ALIASES_REG  "SOFTWARE\\EMWAC\\IMS\\Aliases"
  #define SOFT_LISTS_REG    "SOFTWARE\\EMWAC\\IMS\\Lists"
#endif
#ifdef V3
#ifndef K_SEARCH // K_SEARCH OEM 版
  #define DOMAIN_REG      "SOFTWARE\\KS\\IMS\\Domain"
  #define DOMAIN_SMTPIP   "SOFTWARE\\KS\\IMS\\Domain\\SmtpIP"
  #define DOMAIN_POP3IP   "SOFTWARE\\KS\\IMS\\Domain\\Pop3IP"
  #define DOMAIN_FOLDER   "SOFTWARE\\KS\\IMS\\Domain\\Folder"
#else
  #define DOMAIN_REG      "SOFTWARE\\SPA-Pro\\IMS\\Domain"
  #define DOMAIN_SMTPIP   "SOFTWARE\\SPA-Pro\\IMS\\Domain\\SmtpIP"
  #define DOMAIN_POP3IP   "SOFTWARE\\SPA-Pro\\IMS\\Domain\\Pop3IP"
  #define DOMAIN_FOLDER   "SOFTWARE\\SPA-Pro\\IMS\\Domain\\Folder"
#endif
#else
  #define DOMAIN_REG      "SOFTWARE\\SPA\\IMS\\Domain"
  #define DOMAIN_SMTPIP   "SOFTWARE\\SPA\\IMS\\Domain\\SmtpIP"
  #define DOMAIN_POP3IP   "SOFTWARE\\SPA\\IMS\\Domain\\Pop3IP"
  #define DOMAIN_FOLDER   "SOFTWARE\\SPA\\IMS\\Domain\\Folder"
#endif
#ifdef K_SEARCH // K_SEARCH OEM 版
  #define MAIL_SPOOL        "%SystemDrive%\\ks_mail"
#else
  #define MAIL_SPOOL        "%SystemRoot%\\SYSTEM32\\SPA\\MAIL"
#endif
#endif

#define MAIL_BOX           "%HOME%\\INETMAIL\\INBOX" 
#define EVENT_REG          "SYSTEM\\CurrentControlSet\\Services\\Eventlog\\Application\\"
#define SYSTEM_REG_DS      "SYSTEM\\CurrentControlSet\\Services\\" SMTP_SERVICE_DS
#define SYSTEM_REG         "SYSTEM\\CurrentControlSet\\Services\\" SMTP_SERVICE
#define SYSTEM_PARAM_REG   "SYSTEM\\CurrentControlSet\\Services\\" SMTP_SERVICE "\\Parameters"
#define SYSTEM_IMS_PARAM_REG   "SYSTEM\\CurrentControlSet\\Services\\SMTPRS\\Parameters"
#define SMTP_NAME       TRADEMARK SOFTNAME
#define ESMTP_NAME      TRADEMARK ESOFTNAME
#define SMTP_DEBUG_MESS ESMTP_NAME " %s %s %s"

#ifdef USE_STARTTLS
#define SMTP_STLS "STARTTLS"
#endif
#define SMTP_HELO "HELO" // HELO <SP> <domain> <CRLF>
#ifdef ESMTP_AUTH
  #define SMTP_EHLO "EHLO" // HELO <SP> <domain> <CRLF>
  #define SMTP_AUTH "AUTH" // AUTH <SP> <code@domain> <CRLF>
  #define SMTP_PASS "PASS" // ID And PASSWORD<CRLF>
#endif
#define SMTP_MAIL "MAIL" // MAIL <SP> FROM:<reverse-path> <CRLF>
#define SMTP_RCPT "RCPT" // RCPT <SP> TO:<forward-path> <CRLF>
#define SMTP_DATA "DATA" // DATA <CRLF>
#define SMTP_DOT  ".\r\n" // DATA <CRLF>
#define SMTP_RSET "RSET" // RSET <CRLF>
// #define SMTP_SEND "SEND" // SEND <SP> FROM:<reverse-path> <CRLF>
// #define SMTP_SOML "SOML" // SOML <SP> FROM:<reverse-path> <CRLF>
// #define SMTP_SAML "SAML" // SAML <SP> FROM:<reverse-path> <CRLF>
#define SMTP_VRFY "VRFY" // VRFY <SP> <string> <CRLF>
#define SMTP_EXPN "EXPN" // EXPN <SP> <string> <CRLF>
#define SMTP_HELP "HELP" // HELP [<SP> <string>] <CRLF>
#define SMTP_NOOP "NOOP" // NOOP <CRLF>
#define SMTP_QUIT "QUIT" // QUIT <CRLF>
//#define SMTP_TURN "TURN" // TURN <CRLF>
/// It limit a relay smtp receiver. spare

#define SMTP_GOOD_STLS  "220 2.0.0 Ready to start TLS\r\n"
#define SMTP_ERROR_STLS "530 5.7.0 Must issue a STARTTLS command first\r\n"
#define SMTP_HELP_RESP  "211 2.0.0 System status, or system help reply.\r\n"
#define SMTP_HELP_MESS  "214 2.0.0 Help message.\r\n"
#define SMTP_STAT_MESS  "220 %s " SMTP_DEBUG_MESS "\r\n" // %s=domain
#define SMTP_STAT_FAIL  "421 4.7.0 %s %s\r\n"
#define SMTP_QUIT_MESS  "221 2.0.0 %s closing connection.\r\n" // %s=domain
#define SMTP_GOOD_MESS  "250 2.0.0 %s ok.\r\n" // %s=user name
#define SMTP_GOOD_HELO  "250 %s Hello [%s], pleased to meet you\r\n" // %s=user name
//認証テストサンプル
//  AUTH PLAIN AGthd2EAQUNDRVNTMzA=
//#define SMTP_GOOD_EHLO       "250-%s Hello [%s], pleased to meet you\r\n250 AUTH %s\r\n" // %s=user name
#define SMTP_GOOD_EHLO       "250 %s Hello [%s], pleased to meet you\r\n"
#define SMTP_GOOD_EHLO_STLS  "250 STARTTLS\r\n" // starttls command
#define SMTP_GOOD_EHLO_SIZE  "250 SIZE %ld\r\n" // %s=max size
#define SMTP_GOOD_EHLO_AUTH  "250 AUTH %s\r\n"  // %s=user name
#define SMTP_GOOD_EHLO_SUBMITTER "250 SUBMITTER\r\n"  // Sender ID
#define SMTP_GOOD_EHLO_8BITMIME "250 8BITMIME\r\n"  // 8BITMIME
#define SMTP_GOOD_AUTH  "334 %s\r\n" // %s=user name
#define SMTP_GOOD_PASS  "235 2.0.0 Authentication successful.\r\n"
//#define SMTP_GOOD_PASS  "250 Authentication successful.\r\n"
#define SMTP_GOOD_MAIL  "250 2.0.0 <%s>... Sender ok.\r\n" // %s=user name
#define SMTP_GOOD_RCPT  "250 2.0.0 <%s>... Recipient ok.\r\n" // %s=user name
#define SMTP_GOOD_RSET  "250 2.0.0 Reset state\r\n"
#ifdef UPDATE_20071209B // VRFY,EXPN命令の非応答設定での応答コードを252変更。
#define SMTP_HIDDEN_VRFY  "252 2.0.0 Cannot VRFY user; try RCPT to attempt delivery\r\n"
#endif
#ifdef UPDATE_20071209 // EXPN命令でメーリングリストメンバが正しく応答されないことがある不具合。
#define SMTP_GOOD_VRFY  "250 2.0.0 %s <%s>\r\n"
#define SMTP_GOOD_EXPN  "250 2.0.0 <%s>\r\n"
#define SMTP_GOOD_EXPN2 "250-<%s>\r\n"
#define SMTP_BAD_EXPN   "550 5.1.1 That is a user name, not a mailing list\r\n"
#else
#define SMTP_GOOD_VRFY  "250 2.0.0 %s %s ... User ok\r\n"
#define SMTP_GOOD_EXPN  "250 2.0.0 %s %s\r\n"
#endif
//#define SMTP_BAD_RCPT   "251 User not local; will forward to <%s>.\r\n" // %s=forward-path
//#define SMTP_BAD_RCPT   "450 Please mail to a valid e-mail address.\r\n" // %s=forward-path
#define SMTP_BAD_RCPT   "550 5.1.1 Please mail to a valid e-mail address.\r\n" // %s=forward-path
#ifdef UPDATE_20200131 // セキュアハンドラのブラックリストで拒絶した場合のコードを追加
#define SMTP_BAD_FROM_APPROVAL   "550 5.1.1 Please mail to a valid e-mail address. approval filter. '%s' is entry in the blacklist.\r\n" // %s=forward-path
#endif
#define SMTP_MAX_RCPT   "550 5.1.1 Address exceeds a maximum.\r\n"
#define SMTP_FULL_RCPT  "450 4.2.2 Sorry. e-mail address box is full.\r\n" // %s=forward-path
#define SMTP_DEVICE_FULL  "450 4.3.1 Sorry. disk space not enough.\r\n" // %s=forward-path
#define SMTP_MAILFILTER_STOP  "450 4.3.1 Sorry. unconnected mail filter.\r\n"
#define SMTP_DATA_MESS  "354 Start mail input;id <B%010lu> end with <CRLF>.<CRLF>\r\n"
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
#define SMTP_DATA_MESS2 "354 2.0.0 Start mail input;id <%s@%s> end with <CRLF>.<CRLF>\r\n"
#endif
#ifdef K_SEARCH // K_SEARCH OEM 版
#define SMTP_DATA_MESS2 "354 2.0.0 Start mail input;id <%s@%s> end with <CRLF>.<CRLF>\r\n"
#endif
#define SMTP_BAD_MESS   "421 4.3.2 %s Service not available, closing transmission channel.\r\n" // %s=domain
//#define SMTP_BAD_FROM   "450 Please mail from a valid e-mail address.\r\n"
//#define SMTP_BAD_IP     "450 Please mail from a valid IP or Domain address.\r\n"
#define SMTP_BAD_SPF    "450 4.7.1 Sorry. mail from a valid IP or Domain address.(SPF_GRAY)\r\n"
#define SMTP_BAD_FROM   "550 5.1.7 Please, mail from a valid e-mail address.\r\n"
#define SMTP_BAD_IP     "550 5.1.7 Please, mail from a valid IP or Domain address.\r\n"
#define SMTP_BAD_NOAUTHML "550 5.7.0 Please, mail after SMTP-authentication.\r\n"
#define SMTP_BAD_ORDBLIST "550 5.7.0 Sorry. Your IP is in the open relay data base list.\r\n"
// #define "450 Requested mail action not taken: mailbox unavailable [E.g., mailbox busy].\r\n"
// #define "451 Requested action aborted: error in processing.\r\n"
// #define "452 Requested action not taken: insufficient system storage.\r\n"
#define SMTP_UNKNOW_MESS    "500 5.5.2 command unrecognized: \"%s\"\r\n" // %s=unnon token
#define SMTP_UNKNOW_HELP    "500 5.5.2 Unknown or unimplemented command\r\n"
#define SMTP_BAD_ARGUMENT   "501 5.5.2 Argument required\r\n"
#define SMTP_BAD_DOMAIN     "501 5.0.0 Invalid domain name\r\n"
#define SMTP_BAD_PHASE      "501 5.5.2 Syntax error in parameters or arguments.\r\n"
#define SMTP_HELO_NODOMAIN  "501 5.5.2 HELO requires domain address\r\n"
#define SMTP_AUTH_CANCEL    "501 5.0.0 The client wanted to cancel authentication.\r\n"
#define SMTP_ISNT_INPLEMENTED "502 5.3.3 Command isn't implemented.\r\n"
#define SMTP_HELO_DUPLICATE "503 5.5.1 %s Duplicate HELO\r\n"
#define SMTP_EHLO_DUPLICATE "503 5.5.1 %s Duplicate EHLO\r\n"
#define SMTP_AUTH_DUPLICATE "503 5.5.1 %s Duplicate AUTH\r\n"
#define SMTP_MAIL_DUPLICATE "503 5.5.1 Sender already specified\r\n"  // Duplicate MAIL FROM:
#define SMTP_AUTH_NEED      "503 5.0.0 Need EHLO before AUTH\r\n"  // before AUTH:
#define SMTP_MAIL_NEED      "503 5.0.0 Need AUTH before MAIL\r\n"  // before MAIL
#define SMTP_RCPT_NEED      "503 5.0.0 Need MAIL before RCPT\r\n"  // before RCPT TO:
#define SMTP_NEED_COMMAND   "503 5.0.0 Need %s\r\n"
// #define "503 Bad sequence of commands.\r\n"
#define SMTP_AUTH_NOT_ENABLED "503 5.5.1 Error: authentication not enabled.\r\n"
#define SMTP_AUTH_BAD_TYPE  "504 5.7.4 Unrecognized authentication type.\r\n"
#define SMTP_PASS_FAIL      "535 5.7.0 Authentication failed.\r\n"
// #define "504 Command parameter not implemented.\r\n"
// #define "550 Requested action not taken: mailbox unavailable [E.g., mailbox not found, no access].\r\n"
#define SMTP_UNKNOW_USER     "550 5.1.1 <%s> ... User unknown.\r\n"
#define SMTP_BAD_USER        "551 5.0.0 User not local; please try <%s>.\r\n" //%s=forward-path
#define SMTP_BAD_ADDR        "550 5.7.1 Sorry %s, I don't allow unauthorized relaying.\r\n"
#define SMTP_BAD_DATARCPT    "552 5.5.1 Requested mail action aborted: need RCPT.\r\n"
#define SMTP_BAD_DATASIZE    "552 5.3.1 Requested mail action aborted: exceeded storage allocation.\r\n"
#define SMTP_BAD_DATAVIRUS   "552 5.7.1 Requested mail action aborted: caught by the filter setting.\r\n"
#define SMTP_BAD_MAILFILTER  "552 5.7.1 Requested mail action aborted: unconnected mail filter.\r\n"
#ifdef UPDATE_20181026 // ヒットしたブラックリストファイルの文字列位置情報を追加表示する。
#define SMTP_BAD_APPROVAL    "552 5.7.1 Requested mail action aborted: approval filter. Blacklist line (%lu)\r\n"
#else
#define SMTP_BAD_APPROVAL    "552 5.7.1 Requested mail action aborted: approval filter.\r\n"
#endif
//#define SMTP_BAD_DATAFROM    "552 5.7.1 Requested mail action aborted: mail from a valid e-mail address.\r\n"
//#define SMTP_BAD_NOAUTHML    "554 Requested mail action aborted: non-authentication mail.\r\n"
#define SMTP_BAD_DATARECEIVED "554 5.4.6 too many Received: headers.\r\n"
//RFC2821に規定メールアドレス+ドメインの最大長は、255Byteまで
#define SMTP_LONG_ADDRESS    "553 5.1.0 <%s> too long (255 bytes max)\r\n"



//#define "553 Requested action not taken: mailbox name not allowed [E.g., mailbox syntax incorrect].\r\n"
//#define "554 Transaction failed.\r\n"

#ifdef BTHREAD
BOOL HeloDispatch(PCLIENT_CONTEXT lpClientContext);
#ifdef ESMTP_AUTH
BOOL EhloDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL AuthDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL PassDispatch(PCLIENT_CONTEXT lpClientContext);
#endif
BOOL MailDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL RcptDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL DataDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL DotDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL RsetDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL VrfyDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL ExpnDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL HelpDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL NoopDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL QuitDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL BadDispatch(PCLIENT_CONTEXT lpClientContext);
#else
SMTPRSDisposition
HeloDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );

#ifdef ESMTP_AUTH

SMTPRSDisposition
EhloDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );

SMTPRSDisposition
AuthDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );

SMTPRSDisposition
PassDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );

#endif

SMTPRSDisposition
MailDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );

SMTPRSDisposition
RcptDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );

SMTPRSDisposition
DataDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );

SMTPRSDisposition
DotDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );

SMTPRSDisposition
RsetDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );

/*
SMTPRSDisposition
SendDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );

SMTPRSDisposition
SomlDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );

SMTPRSDisposition
SamlDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );

*/

SMTPRSDisposition
VrfyDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );

SMTPRSDisposition
ExpnDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );

SMTPRSDisposition
HelpDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );

SMTPRSDisposition
NoopDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );

SMTPRSDisposition
QuitDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );
/*
SMTPRSDisposition
TurnDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );
*/

SMTPRSDisposition
BadDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );
#endif // BTHREAD

#endif
