////////////////////////////////////////////////////////////
// smtp.h Copyright K.kawakami
////////////////////////////////////////////////////////////
#ifndef _SMTP_H
#define _SMTP_H
#pragma warning(disable : 4996)
#include <io.h>

//#define  VC6 // VC6++でコンパイルする場合

//#define  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理

//#define DNS_TEST
//#define DATA_CASH 1
//#define TUNING 1
//#define ENTERPRISE
//#define FOR_BRIDGEGATE // ブリッジゲートＯＥＭ版

//#define READING_ASYNCHRONOUS

#define E_POST // @Solomon 海外バージョン製品名
//#define V5
#define V4
#define V3
#define IPv6
#define LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。

//#define HOME_VERSION   // SSL抜き
#ifdef HOME_VERSION
 #define USER_LIMIT 5 // 5 ユーザ制限
#endif

// 以下 v4.84
#define UPDATE_20260603 // ログ出力の排他処理化を追加
#define UPDATE_20260605 // 詳細ログファイルのオープン失敗リトライ処理の追加
#define UPDATE_20260606 // ログの書き込み信頼性向上対策
/*
  ログの書き込み用ファイルディスクプリタを新規で開いた後保持して
　一定時間間隔（１時間間隔）内の書込みは実行し時間を超えた時の書き
　込みはクローズ再オープンし、書き込み時は、MUTEXで排他する。
*/

// 以下 v4.83
#define UPDATE_20250418 // 初回送信がESMTPで送信拒絶等によるソケットが相手側から切断されている状態になった場合で切断されていると認識出来ずに再送処理が行われ重複メールが発生する不具合
#define UPDATE_20250601 // OAUTH2でのアクセストークンとリフレッシュトークンを特定のメールアドレスを参照先とする機能を追加

// 以下 v4.82
#define UPDATE_20241104 // XOAUTH2でのBearer用のコード生成で余計な文字が含まれている不具合
#define UPDATE_20241110 // XOAUTH2のトークン取得に失敗する不具合
#define UPDATE_20241114 // XOAUTH2のIDとアクセストークンのエンコードに余分なデータが出力される。
#define UPDATE_20241114_BUF // XOAUTH2のアクセストークン長いとログ用バッファオーバフローが発生する。
#define UPDATE_20241124 // OAUTH2での認証方式用にユーザ別認証コード保管ファイル(xoauth2_code.dat)を各アカウントフォルダに設定可能にした
#define UPDATE_20241124_BOX // OAUTH2での認証方式用にアカウント毎にアクセストークンとリフレッシュトークンを格納するようにした
#define UPDATE_20241126 // XOAUTH2で失敗以外の応答コード200番台以外が返ってきた場合の対策
#define UPDATE_20241126_NODEL // XOAUTH2で失敗時のアクセストークン再取得時は、リフレッシュトークンファイルは消さずに残し再利用する。
#define UPDATE_20241127 // XOAUTH2で失敗以外の応答コード300番台以外が返ってきた場合の対策
#define UPDATE_20241127_BOX // OAUTH2でのアクセストークンとリフレッシュトークンの保存先を[メール作業フォルダ/oauth2/メールアドレス]に変更するようにした。
#define UPDATE_20241128 // GATEWAY.DATにエンベロープの送信元の条件も追加可能に拡張した。
#define UPDATE_20241129 // XOAUTH2の認可コードの読出し完了時のファイルのクローズ抜けていた。
#define UPDATE_20241217 // gateway.datにSMTP認証の指定がある時は優先するようにした。
#define UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策

// 以下 v4.81
#define UPDATE_20240127 // ＭＬ制御応答＆送信エラー通知メールにもDKIMサインを追加可能にするオプション
#define UPDATE_20240127A // 転送メールにARCヘッダを追加可能にした。
#define UPDATE_20240127B // 自動応答メールにもDKIMサインを追加可能にするオプション
#define UPDATE_20240127C // DKIMサイン実施無効フラグを追加 (bit0:送信エラー bit1:ＭＬ制御応答 bit2:転送メール bit:3自動応答)
#define UPDATE_20240128 // 転送時に添付除去指定した場合のARCヘッダを整合するようにする
#define UPDATE_20240129 // DKIMサインが追加されない
#define UPDATE_20240130 // 添付削除した転送メールにDKIMサインを付けなおす
#define UPDATE_20240130A // 添付削除した転送メールの転送先が複数あるとその度にサインしないようにする。
#define UPDATE_20240202  // 添付削除した転送メールの転送先が複数あるとその度にサインしないようにする。UPDATE_20240130Aの訂正
#define UPDATE_20240420 // 暗号化通信方式のセキュリティレベルの設定
#define UPDATE_20240510 // メール転送時に転送先を一時的には保存したファイルが削除されずに残ってしまうことがある。

// 以下 v4.80
//#define UPDATE_20231006 // Openssl 3系でdocomoのSMTPに繋がらない対策 意味なし
#define UPDATE_20231007 // STARTTLSで暗号化ネゴシェーションに失敗するドメインリストの作成と失敗先へのプレーン送信への切替対策
#define UPDATE_20231027 // 送信ファイルのファイルシステムからの一覧取得時に最新の状態を取得する対策

// 以下 v4.79
#define UPDATE_20230926 // 次のメールに暗号化通信フラグが一旦初期化されずに、SMTP Over SSLとして処理される場合がある不具合
#define UPDATE_20230927 // 受信直後バッファ内容ののトレース

// 以下 v4.78
#define UPDATE_20230523 // OPENSSLライブラリを3.1.xに変更した。
#define ADD_XOAUTH2 // OAUTH2での認証方式を追加
#define ADD_XOAUTH2_A // 認証トークンからアクセストークン＆認証トークンの自動取得処理
#define ADD_XOAUTH2_B // アクセストークンの保存と再使用
#define ADD_XOAUTH2_C // GATEWAY.DATにOAUTH2のURLを指定可能に

// 以下 v4.77
#define UPDATE_20230105 // X-UIDLヘッダを追加するオプション
#define UPDATE_20230310 // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しない
#define UPDATE_20230425 // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しないとき、内部ドメインも含まれてしまう不具合
#define UPDATE_20230425A // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しないとき、内部ドメインも含まれてしまう不具合

// 以下 v4.54 // 354の時リトライすると送れている場合もあるので複数同じメールが届く可能性があるのでNG
//#define UPDATE_20120308 // 300番台(354)で切断された時もリトライを行う(ページ単位のメモリ確保名なので中止)
// 以下 v4.76
#define UPDATE_20220725 // "MAIL From:","RCPT To:"のコマンド出力を"MAIL FROM:","RCPT TO:"に変更するようにした。

// 以下 v4.75
#define UPDATE_20220414 // ゲートウェイ先ごとにSMTP認証ID,PWを規定可能にした。
#define UPDATE_20220416 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合を有効にするオプションフラグ
#define UPDATE_20220418 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合にデータを暗号化する
#define UPDATE_20220508 // 設定ドメイン毎のＩＰに送信元ＩＰを切替える設定有効時に"Reporting-MTA:"ヘッダを変更するようにした。
#define UPDATE_20231103 // エラーメール生成時にメールヘッダの情報を利用するオプション機能を追加。
#define UPDATE_20231104 // 自動応答生成時にメールヘッダの情報を利用するオプション機能を追加。

// 以下 v4.74
#define UPDATE_20211210_IPV6PREFIX // IPv6アドレスプレフィックスに対応した。

// 以下 v4.73
#define UPDATE_20210704 //配送処理後の"$CP"が削除されずに残らないようにする対策

// 以下 v4.72
#define UPDATE_20210127 // SMTP認証時のパスワード領域上限数を、63byteから127byteに拡張した。(SendGrid SMTP認証対策)
#define UPDATE_20210303 // メーリングリストの投稿内容保存時、題名が複数行にまたがるとインデックス取得でハングする
#define UPDATE_20210303A // メーリングリストで投稿内容保存ファイルの拡張子の指定を可能にするオプションを追加した。
#define UPDATE_20210321 // 添付分離時にMLメンバーが無い場合のエラーメール生成を抑止する

// 以下 v4.71
#define UPDATE_20200609 // メールヘッダの1行がが252byteを以上で不正な改行が不可される

// 以下 v4.70
#define UPDATE_20191024 // 送信先がhosts(Aレコード参照)で解決されるとoutlogに記録されない不具合

// 以下 v4.69
#define UPDATE_20190426 // 利用する暗号アルゴリズムの組み合わせオプション 

// 以下 v4.68
#define UPDATE_20190207 // 独自アカウントデータベース内のパスワードの暗号化

// 以下 v4.67
#define UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換えた（SSL2を廃止し、TSL1.3を使用可能にする為）

// 以下 v4.66
#define UPDATE_20181101 // 接続成功してもどこにも送信されない場合は、MXキャッシュをリセットするようにした。
#define UPDATE_20181120 // シングルパートのメールではcontent-transfer-encodingヘッダの除去を行わない対策
#define UPDATE_20181122 // Content-Transfer-EncodingヘッダとContent-Typeヘッダの出現順位により正しく添付の削除が出来るようにする対策
#define UPDATE_20181126 // boundary内に':'が含まれると新しいヘッダとして誤認識してしまう不具合

// 以下 v4.65
#define UPDATE_20180124 // HTMLメールで添付削除時、メール本体とプレーンデータの"Content-Transfer-Encoding:"の形式がメーラでの表示が正常に行えない不具合
#define UPDATE_20181004 // ドメイン名からの順引きが成功したときのアドレス取得でハングすることがあった。

// 以下 v4.64
#define UPDATE_20171211A // 使用可能な暗号化通信方式のバージョンを規定するオプション 0x1:SSL2禁止 0x2:SSL3禁止 0x4:TSL1禁止 0x8:TSL1.1禁止 0x10:TSL1.2禁止

// 以下 v4.63
#define UPDATE_20170207 //LDAP接続失敗時のリトライ処理の修正
#define UPDATE_20170214 // データ受信前に以前の応答データがクリアされるようにした
#define UPDATE_20170215 // 配送スレッドの変数初期化抜けの修正
#define UPDATE_20170222 // 送信エラー時の送信アドレス管理ファイルのクローズ抜け
//#define UPDATE_20170304_FILENAME_ADD_DATE // 保存ファイルに日付情報付加

// 以下 v4.62
#define UPDATE_20160113 // STARTTLS接続後の接続相手のグリーティングにSTARTTLSが含まれると正常に動作しない不具合
#define UPDATE_20160224 // STARTTLSで初期化失敗時にハングする


// 以下 v4.61
#define UPDATE_20151103 // Openssl 1.0.2dに変更した
#define UPDATE_20151126 // 送信の詳細ログにSTARTTLS実行時の記録が残るようにした。
#define ADD_IDCACHE     // Windows,AD,LDAP連携で問合せキャッシュ機能を追加。(問合せ負荷の軽減策)

// 以下 v4.60
#define UPDATE_20150108 // 送信許可時間の設定機能を追加

// 以下 v4.59
#define UPDATE_20121213 // 自動応答の非応答アドレス設定で対象アドレスの比較に*ワイルドカード指定を可能にした。

// 以下 v4.58
#define UPDATE_20141115 // 独自アカウント運用時で最大文字数を128byteまで拡張可能にする対策をした。

// 以下 v4.57
#define UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
#define UPDATE_20141105 // MXレコード参照失敗した受信をSMTP Gatewayへ転送


// 以下 v4.56
#define UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
#define UPDATE_20140805 // MX問合せ失敗があった場合の対策。(切り詰めエラーを無効にした)
#define UPDATE_20140809 //転送処理時のファイル読込みでバイナリデータが含まれていてもデータが切れてしまわない対策を追加。
//#define NEW_MXQUERY // MX問合せのリトライ処理を追加（上記の対応"UPDATE_20140805"で済んだので不要）

// 以下 v4.55
#define UPDATE_20130819 // ML名を指定した制御命令を無効にするオプションを追加

// 以下 v4.54
#define UPDATE_20121105 // IPv4/v6併用で相手FQDNにIPv6が有効なのにIPv4のみでSMTPサーバが接続可能な場合接続できない不具合
#define UPDATE_20121122 // // メーリングリスト配送先が存在しないローカルユーザ宛の時の配送失敗ログを記録するようにした。

// 以下 v4.53
#define UPDATE_20110909 // ML名+ドメイン名が53byteを超えるとハングする

// 以下 v4.52
#define UPDATE_20110525 // gateway.datにIPv6アドレスを設定するとアドレスが正しく認識されない

// 以下 v4.51
#define UPDATE_20110202 // 自動転送の添付削除でHTMLメールに添付があると正しく削除されない

// 以下 v4.50
#define UPDATE_20101126 // ML配送用作業フォルダが作成できない場合に作成リトライを行なう処理を追加

// 以下 v4.49
#define UPDATE_20100118 // 過去の同報送信での送信エラー情報がクリアされずに新たなセッションでの送信エラーにその情報が反映されてしまう可能性があった。

// 以下 v4.48
#define UPDATE_20091224 // 前バージョン環境変数%systemroot%の対策ロジックのミス
  //#define UPDATE_20091210 // 送信エラーの返信メール送信時にハングする可能性
#define UPDATE_20091222A // 異常に長いメールアドレスの処理でハングする可能性
  //#define UPDATE_20091225 // スレッド数を超えて起動してしまう不具合
  //#define UPDATE_20091225A // スレッド作成エラーに対する対策
  //#define UPDATE_0x15639_HUNG_DEBUG // 0x15639ハング調査用
#define UPDATE_0x15639_HUNG_DEBUG_A // 0x15639ハング調査用
  //#define CS_RETURNMAIL // ReturnMail()中の割り込み処理禁止


// 以下 v4.47
//#define UPDATE_20090731 // STARTTLS有効でリクエストに対する応答が拒否の場合、平文で送信継続する
//#define UPDATE_20090903 // メール作業領域で環境変数%systemroot%があるとドライブチェックが正しく出来ない
//#define UPDATE_20090925 // メーリングリスト宛の制御メールの本文がBase64にエンコードされる場合の対策
#define UPDATE_20091014 //  深い階層のアカウントが読めるように対策
#define UPDATE_20091014A // ドメイン無しアカウントでホームディレクトリ参照時ハングする可能性がある
#define UPDATE_20091014C // LDAPへのアカウント問合せ処理の高速化
#define UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
#define UPDATE_20091101 // HOMEディレクトリタイプで無いならホームフォルダの検索をしないようにしAD連携時の処理速度の向上を図った。
#define UPDATE_20091103 // LDAP連携専用のリトライ回数指定の変数に変更

// 以下 v4.46
#define UPDATE_20090610 // %HOME%が設定されているとまともに動かない
#ifdef USE_SSL
#define USE_STARTTLS // STARTTLSコマンドの対応
#endif

#ifndef USE_SSL
#undef UPDATE_20171211A
#undef UPDATE_20190426
#undef UPDATE_20240420
#endif

// 以下 v4.45
#define UPDATE_20080708 // 外部ドメインの転送だけ記録
#define UPDATE_20080704 // 転送指定があると、SMTP情報が書き換わってoutlocallogに記載されてしまう
#define UPDATE_20080626 // 転送先アドレスの先頭に全角空白や半角空白が含まれている場合は、転送処理をスキップするようにした。
#define UPDATE_20080618 // LGWAN機能が有効なとき、ドメインなしアカウントがあるとハングする不具合

// 以下 v4.44
#define LGWAN_ON // 20080603 LGWAN使用時の振分け機能

// 以下 v4.43
#define UPDATE_20080414 // 送信元アドレスが記載されている場合は無視する

// 以下 v4.42
#define UPDATE_20080213 // ドメイン無しアドレスの外部転送で転送できない場合にbaddomainフォルダ内のメールがリトライし続ける

// 以下 v4.41
#define UPDATE_20080111 // MSCSの時、フェールオーバー遷移中のディスクアクセス処理の修正
#define UPDATE_20071217A // 「リトライ間隔のまま」オフでもリトライ間隔で処理される不具合
#define UPDATE_20071217 // domainsフォルダにRCPだけ残骸として残ってリトライ処理が進まない対策
#define UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
#define UPDATE_20071205 //  外部メールアドレス長が255Byteのときの対策メールアドレス長が256Byteのときの対策
#define UPDATE_20071205A // リトライキュー保管時にドメイン名が64バイト以上の場合MD5のハッシュ値に変換したフォルダ名にする

// 以下 v4.40
#define UPDATE_20071208 // 同報メールで送信成功が１つでもあるとすぐにエラーアドレスがエラー生成してしまう
#define UPDATE_20071207 // 受信拒否時のリトライ回数が多くなる
// 以下 v4.39
#define UPDATE_20071129A // MXキャッシュファイルが削除されない場合の対策
#define UPDATE_20071129 //DATA送信後の受信拒否メッセージに送信先アドレスが含まれている場合正しくアドレス抽出できない場合がある。
#define UPDATE_20071123 // 同一宛先の複数ゲートウェイ先（負荷分散）指定対応
// 以下 v4.38
#define UPDATE_20071115 //incomingキュー走査で、同名ファイルの拡張子".$CP"と".RCP"が両方発生する場合の対策
//#define UPDATE_20071113 // 属性が隠しファイルでも処理を可能にする
#define UPDATE_20071022 // ドメイン無しのアカウントはデフォルトドメインを指定する
#define UPDATE_20071016 // LDAPサーバーへの接続リトライ
#define UPDATE_20071009 // senderlogへの出力内容を減らした。
// 以下 v4.37
#define UPDATE_20070907 // faillogにエラーコード記載
#define UPDATE_20070621 // ユーザ毎のSMTP認証ID/PWを設定可能にする
#define UPDATE_20070620 // ドメインごとにDNを設定可能にする
// 以下 v4.36
#define UPDATE_20060615 // LDAPサーバーへの接続リトライ
#define UPDATE_20070608 // メッセージ配送バッファのクリア 最後の行に同じメッセージが出力されてしまう不具合
#define UPDATE_20070521 // OSの予約語対策
#define UPDATE_20070520 // ＭＬメンバでワイルドカード指定が出来ないため、"@"をワイルドカードに流用
#define UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
// 以下 v4.34 
#define UPDATE_20070426 // 更新時間が設定されていない場合はMXキャッシュのデータ設定しない
#define UPDATE_20070425 // MSCSのスタンバイ側に対応
#define UPDATE_20070423 // 起動時にMXキャッシュ情報をクリアする
#define UPDATE_20070405 // イベントログデータベースを追加。
#define UPDATE_20070321 //DNSへの問合せテーブル初期化(初期化が無いと問合せでハングする可能性有り)
//#define UPDATE_20070320A  // Domain.mriの再書込みで実体化せずハングする
//#define UPDATE_20070320  // MXキャッシュデータの削除でハングする
//#define UPDATE_20070319D // RCPファイルの作成完了を確認するまでウエイトする
//#define UPDATE_20070319C // データが空ならキャラクタ検索で見つからない場合ハングする
#define UPDATE_20070319B // キャラクタ検索で見つからない場合ハングする
//#define UPDATE_20070319A // ファイル確認でハングさせない対策
//#define UPDATE_20070319 // MXレコード参照でハングさせない対策
#define UPDATE_20070301 // 対象アドレスの比較 *ワイルドカード有効にする 2007.03.01
#define UPDATE_20070208 // domainsフォルダ内の$LockFileを一旦クリアし対象の処理をスキップ
#define UPDATE_20070130 // 配送不能メールの送信者アカウントの指定をオプションを追加
//#define UPDATE_20070122A // 接続エラー（10055）で接続をリトライする処理を追加
#define UPDATE_20070122 // 設定ドメイン毎のＩＰに送信元ＩＰを切替える設定の修正
#define UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
#define UPDATE_20061128 // リネーム要求が繰り返された場合もウエイトしない
#define UPDATE_20061118 // エラーメールのエンベロープのFROM:を空白にするオプション
#define UPDATE_20060929 // 接続不能時に別のMXレコード選択
#define UPDATE_20060905 // 対象フォルダごとにロックファイルを設定する
#define UPDATE_20060904C // 送信先回線エラーで対象ドメインの保存（MXキャッシュ削除用）
#define UPDATE_20060904B // 送信要求が連続して発生しているとＭＬの処理がウエイトされてしまう。
//#define UPDATE_20060904A // ロックファイルの作成確認ログ出力用
//#define UPDATE_20060904 // ロックファイルの作成確認
#define UPDATE_20060901 // listsフォルダのクリアが出来ない
#define UPDATE_20060725 // 配送ログへ出力フラグの初期化
#define UPDATE_20060716A // 制御ML名に付加されているドメイン名の記述が削除された返信内容になる。
#define UPDATE_20060716 // 存在するMLの先頭文字が一致してしまうと、存在しないML名が登録できてしまう。
#define UPDATE_20060712 // // リトライ間隔のままがオフのときのリトライ処理の修正
#define UPDATE_20060609 // ソケット接続失敗時のエラーメールに記載するメッセージ
#define UPDATE_20060629 // 400番台のリトライが500番台に切替えられるてしまう。
#define UPDATE_20060628 // deadフォルダに保管しないオプション
#define UPDATE_20060624 // holding内のMSGファイルもチェック
#define UPDATE_20060621 // リトライ間隔のままのリトライがスレッドが生きていり間リトライされ続けてしまう対策
#define UPDATE_20060616 // エイリアスの場合実アカウントに変換されてしまう。
#define UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
#define UPDATE_20060613 // senderlogでプロトコルログ のみ出力するオプション追加
#define UPDATE_20060606 // ドメインコントローラへの接続リトライ
#define UPDATE_20060529 // 設定ドメイン毎のＩＰに送信元ＩＰを切替える設定
#define UPDATE_20060513 // ＭＬ管理メールにキーコードを付加
#define UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
#define UPDATE_20060508 // 転送エラーメールの送信MSGとRCPファイルが孤児化してしまう。
#define UPDATE_20060329 // SSLの初期化に失敗すると以降でハングする
#define UPDATE_20060323 // SSLでの送信は、2048byte単位で送信するようにした。
#define UPDATE_20060322 // SSLの初期化に失敗すると以降でハングする
#define UPDATE_20060317 // 管理人通知のメッセージで余分なメッセージが追加される
#define UPDATE_20060116 // バッファオーバーフロー対策
#define UPDATE_20050901 // エラーメールを添付として送信　デフォルト FALSE:しない TRUE:する
#define UPDATE_20050623 // 高負荷で処理中にスレッド数カウンタの不整合が発生する不具合
#define UPDATE_20050609 // MLフォルダのアクセス競合対策
#define UPDATE_20050526 // 管理人宛へのMLアドレスが"-"の含まれるドメインだとおかしくなる。
#define UPDATE_20050518 // SSLデータ読み込み時のタイムアウトチェックの処理修正
#define UPDATE_20050428 // 同報メールアドレスの条件により最後のアドレスが送られない
#define UPDATE_20050126 // Listsに展開されたRCPのリネーム同期対策
#define UPDATE_20050124 // 相手SMTP接続応答待ち時間が長い場合の対策 V4以降のみ
#define UPDATE_20050123 // INCOMING/DOMAINS/LISTSフォルダ取得競合対策（排他ファイルでロック処理）
//#define UPDATE_20050122 // リトライフォルダ(domains)のアクセス競合対策
//#define UPDATE_20050121 // StopLogの記録処理追加
#define UPDATE_20041126
#define UPDATE_20041105
#define UPDATE_20041008
#define UPDATE_20040525
#define UPDATE_20040522
#define UPADTE_20040518
#define UPADTE_20040422
#define UPADTE_20040315
#define UPADTE_20040202 1
#define ML_ASTER_OPTION 1
#define Y2038_BUG 1
#define AUTOPROCESS 1

#define WAIT_TIME 0 //10
#ifdef E_POST
  #define REGTOFILE
  #define CLUSTERING
#endif

#ifdef V4
  //#define REGTOFILE
  //#define CLUSTERING
  #define BEGINLOW  "V4BeginLow"
  #define BEGINHIGH "V4BeginHigh"
  #define LIMITKEY  "V4LimitKey"
#else
#ifdef V3
#ifdef HOME_VERSION
  #define BEGINLOW  "V3@HomeBeginLow"
  #define BEGINHIGH "V3@HomeBeginHigh"
  #define LIMITKEY  "V3@HomeLimitKey"
#else
  #define BEGINLOW  "V3BeginLow"
  #define BEGINHIGH "V3BeginHigh"
  #define LIMITKEY  "V3LimitKey"
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
#define WS2IP6_INCLUDED
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
/*
const char * WSAAPI
inet_ntop(
    int AddressFamily,     // Address family to which the address belongs.
    const void *Address,   // Address (binary) to convert.
    char *OutputString,    // Where to return the output string.
    int OutputBufferSize);
*/
int WSAAPI
inet6_addr(
    const char *InputString,   // IPv6 address (in "colon" representation).
    struct in6_addr *Address);  // Where to return binary representation.
#endif
#define  SPA 1
#endif

#include <windows.h>
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
#include <winldap.h>
#endif
#include <lmcons.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <tchar.h>
#include <locale.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_SSL
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif
#include "profile.h"

#define MAX_FILE 50
#define LOG_BUFFER 2048
//#define TMQ_ON 1
//#define TEST_MODE 1

#define LOCKFILE        "$LockFile"
#define ADCACHE         "adds"
#ifdef E_POST
#ifdef FOR_BRIDGEGATE
#define SOFTNAME       "SMTP Delivery Agent for BridgeGate" 
#else
#define SOFTNAME       "SMTP Delivery Agent"
#endif
#else
#ifdef V4
#define SOFTNAME       "SMTP Delivery Agent @Solomon" 
#else
#ifdef HOME_VERSION
#define SOFTNAME       "SMTP Delivery Agent @Home" 
#else
#define SOFTNAME       "SMTP Delivery Agent"
#endif
#endif
#endif

#define LDAPOPTION_REG  "SOFTWARE\\EPOST\\LDAPOPTION"

#ifdef E_POST
  #define ESMTP_ON 1
  #define TRACE_MODE 1
  //#define WAX_ON 1
  #define ANNOUNCE 1
  #define TRADEMARK       "E-POST " //"Premix "
  #define SMTP_SERVICE    "EPSTDS"
  #define SMTP_SERVICE_RS "EPSTRS"
  #define SOFT_REG          "SOFTWARE\\EMWAC\\IMS"
  #define SOFT_ALIASES_REG  "SOFTWARE\\EMWAC\\IMS\\Aliases" //"SOFTWARE\\EPOST\\IMS\\Aliases"
  #define SOFT_LISTS_REG    "SOFTWARE\\EMWAC\\IMS\\Lists"   //"SOFTWARE\\EPOST\\IMS\\Lists"
  #define DOMAIN_REG        "SOFTWARE\\EPOST\\IMS\\Domain"
  #define DOMAIN_MTAIP      "SOFTWARE\\EPOST\\IMS\\Domain\\MTAIP"
  #define DOMAIN_SMTPIP     "SOFTWARE\\EPOST\\IMS\\Domain\\SmtpIP"
  #define DOMAIN_POP3IP     "SOFTWARE\\EPOST\\IMS\\Domain\\Pop3IP"
  #define DOMAIN_FOLDER     "SOFTWARE\\EPOST\\IMS\\Domain\\Folder"
  #define DOMAIN_BASEDN     "SOFTWARE\\EPOST\\IMS\\Domain\\BDN"
  #define MAIL_SPOOL        "%SystemRoot%\\SYSTEM32\\E-POST\\MAIL"
#else
  #define ESMTP_ON 1
  #define TRACE_MODE 1
#ifdef V3
  #define TRADEMARK       "SPA-PRO "
  #define SMTP_SERVICE    "SPADS-PRO"
  #define SMTP_SERVICE_RS "SPARS-PRO"
#else
  #define TRADEMARK       "SPA "
  #define SMTP_SERVICE    "SPADS"
  #define SMTP_SERVICE_RS "SPARS"
#endif
  #define SOFT_REG        "SOFTWARE\\EMWAC\\IMS"
  #define SOFT_ALIASES_REG  "SOFTWARE\\EMWAC\\IMS\\Aliases"
  #define SOFT_LISTS_REG    "SOFTWARE\\EMWAC\\IMS\\Lists"
#ifdef V3
  #define DOMAIN_REG      "SOFTWARE\\SPA-Pro\\IMS\\Domain"
  #define DOMAIN_MTAIP    "SOFTWARE\\SPA-Pro\\IMS\\Domain\\MTAIP"
  #define DOMAIN_SMTPIP   "SOFTWARE\\SPA-Pro\\IMS\\Domain\\SmtpIP"
  #define DOMAIN_POP3IP   "SOFTWARE\\SPA-Pro\\IMS\\Domain\\Pop3IP"
  #define DOMAIN_FOLDER   "SOFTWARE\\SPA-Pro\\IMS\\Domain\\Folder"
#else
  #define DOMAIN_REG      "SOFTWARE\\SPA\\IMS\\Domain"
  #define DOMAIN_SMTPIP   "SOFTWARE\\SPA\\IMS\\Domain\\SmtpIP"
  #define DOMAIN_POP3IP   "SOFTWARE\\SPA\\IMS\\Domain\\Pop3IP"
  #define DOMAIN_FOLDER   "SOFTWARE\\SPA\\IMS\\Domain\\Folder"
#endif
  #define MAIL_SPOOL        "%SystemRoot%\\SYSTEM32\\SPA\\MAIL"
#endif

#define MAIL_BOX          "%HOME%\\INETMAIL\\INBOX" 
#define EVENT_REG          "SYSTEM\\CurrentControlSet\\Services\\Eventlog\\Application\\"
#define SYSTEM_REG       "SYSTEM\\CurrentControlSet\\Services\\" SMTP_SERVICE
#define SYSTEM_PARAM_REG SYSTEM_REG "\\Parameters"
#define SYSTEM_REG_RS    "SYSTEM\\CurrentControlSet\\Services\\" SMTP_SERVICE_RS
#define SYSTEM_PARAM_REG_RS SYSTEM_REG_RS "\\Parameters"
#define SMTP_NAME       TRADEMARK SOFTNAME
#define SMTP_DEBUG_MESS SMTP_NAME " %s %s %s"

struct _RCP {
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
  char mMID[512];
  char mMIDNo[512];
  char mMNo[512];
#ifdef SENDERID
  char mSubmitter[512];
#endif
  char mFrom[512];
  char mTo[512];
  char mCurrentTo[512];
  char mDomain[512];
  char mSmtp[1025];
  char mSmtpIP[16];
#ifdef UPDATE_20080704 // 転送指定があると、SMTP情報が書き換わってoutlocallogに記載されてしまう
  char mBSmtp[1025];
  char mBSmtpIP[16];
#endif
  char mMsg[512];
  char mRcp[512];
  char mToOK[512];
  char mToNG[512];
#else
  char mMID[256];
  char mMIDNo[256];
  char mMNo[256]; //[16];
#ifdef SENDERID
  char mSubmitter[256];
#endif
  char mFrom[256];
  char mTo[256];
  char mCurrentTo[256];
  char mDomain[256];
  char mSmtp[1025];
  char mSmtpIP[16];
#ifdef UPDATE_20080704 // 転送指定があると、SMTP情報が書き換わってoutlocallogに記載されてしまう
  char mBSmtp[1025];
  char mBSmtpIP[16];
#endif
  char mMsg[256];
  char mRcp[256];
  char mToOK[256];
  char mToNG[256];
#endif
  DWORD nToOK;
  DWORD nToNG;
  char *fsts;
  FILE *fp;
  char mLogTo[2048];
  BOOL bHaed;
  BOOL bGreeting;
  BOOL bNego;
  char mAnswer[2048];
  char mRCPAnswer[2048];
  int  nConnectRetry;
  int  nPort;
  PHOSTENT phe;
#ifdef UPDATE_20150108 // 送信許可時間の設定機能を追加
  BOOL bUseTime;
#endif
  BOOL bUsedSSL;
#ifdef USE_SSL
  SSL_CTX *ctx;
  SSL  *ssl;
#endif
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
  int  nGateAuthType; //  0x1:LOGIN 0x2:PLAIN 0x4:CRAM-MD5 0x8:XOAUTH2 gateway.datでのSMTP認証情報
#endif
#ifdef V4
#ifdef UPDATE_20050124
  FILETIME   grst;
  FILETIME   gret;
#endif
#endif
#ifdef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
  CHAR mSetRetryMode[16];
#endif
#ifdef DATA_CASH
	char                mFsbuf[0x4000]; //0x7ffe];  // ファイルアクセスバッファの
	char                mFrbuf[0x4000]; //0x7ffe];  // ファイルアクセスバッファの
	char                mFwbuf[0x4000]; //0x7ffe];  // 有効範囲は 2 < size < 32768 
#endif
};

struct _MRI {
  char mDom[1025+14];
  char mLastTry[32];
  char mTryCount[32];
  char mErrCode[32];
};

struct _MAIL_CTL {
  int  nMode;           // 0:なし,1:自動転送,2:自動応答
  char Subject[256];    // 返信タイトル
  char ForwardSubject[256]; // 送信者アドレス通知タイトル
  char RecordForward[256];  // 自動応答時の送信者アドレス通知先のアドレス
  char ReplyFrom[256];  // 返信元アドレス
  char ReplyTo[256];    // 返信先アドレス
  char ForwardFrom[256]; // 転送先アドレス
  char ForwardTo[2048]; // 転送先アドレス
  BOOL bDiscard;        // BOXへの保管の有無
  BOOL bRecordReplies;  // 自動応答メールアドレスの記録の有無
  BOOL bRecordDate;     // 自動応答年月日の記録の有無
  BOOL bCopyReplies;    // 自動応答メールアドレスをNOREPLY.TXTにもコピーする
  BOOL bEchoSubject;    // 受信タイトルのエコー
  BOOL bEchoMessage;    // 受信内容のエコー
  BOOL bDeleteHeader;   // エコー内容にヘッダーを含むか否か
  BOOL bDataDiscard;    // 転送時に添付ファイルを破棄するか否か、0(no):破棄しない、1(yes):破棄する
  BOOL bDataSize;       // 転送時の本文データサイズ制限指定の有無
  DWORD nDataSize;      // 転送時の本文データサイズ制限 byte単位、bBodyDiscard=1の場合のみ有効
#ifdef AUTOPROCESS
  BOOL bAutoProcess;    // 翻訳実行フラグ FALSE:実行しない TRUE:実行する
  char Module[256];     // 翻訳実行モジュール
  char CharSet[256];    // 翻訳結果のキャラクタセット us-ascii,iso-2022-jp.... etc
#endif

  // IMS.CTL            // 制御ファイル名
  // AUTORPLY.TXT       // 自動応答ファイル
  // NOREPLY.TXT        // 自動応答しないメール一覧
  // YESREPLY.TXT       // 自動応答するメール一覧
};

#define MAXDNAME 1025

struct _SMTPDS {
  DWORD dwNo;
  DWORD dwWait;
  char  mNS[MAXDNAME];
  char  mMQ[MAXDNAME];
};

typedef struct ldap_data
{
  char *pHost;
  DWORD nPort;
  char *pLogonDomain;
  char *pLogonID;
  char *pLogonPW;
  char *pScope;
  char *pMailAddr;
  char *pRequest1;
  char *pRequest2;
  char *pAnswer;
} LDAPD;

#endif