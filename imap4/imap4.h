////////////////////////////////////////////////////////////
// imap4.h Copyright K.kawakami
////////////////////////////////////////////////////////////
#ifndef _IMAP4_H
#define _IMAP4_H
#include <direct.h>
#pragma warning(disable : 4996)

//#define VC6 // VC6++でコンパイルする場合

//#define BETA   1  // ベータ版
//#define TUNING 1
//#define ENTERPRISE
//#define FOR_BRIDGEGATE // ブリッジゲートＯＥＭ版

//#define READING_ASYNCHRONOUS

//#define SOCKET_FAIL_TEST // 強制テスト：テスト的にエラーを発生 ソケット無応答状態の動作調査用

#define E_POST // E-POST OEM版
//#define V5
#define V4
#define V3
#define IPv6
#ifndef NO_LDAP
#define LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
#endif
#ifdef LDAP_ON
#define LDAP_OPTION_LICENSE // LDAP Optionのライセンスチェックを行なう
#endif

//#define HOME_VERSION   // SSL抜き
#ifdef HOME_VERSION
 #define USER_LIMIT 5 // 5 ユーザ制限
#endif

#define LOG_BUFFER 2048
#define MAX_ACCOUNT_LENGTH  256 //30 // アカウントの最大バイト数 max64 RFC5321
#define BODY_PART_LIMIT 150// 20  // FETCH命令のBODYパートとして解釈する上限

//#define ADD_UNLIMITED
//#define ADD_ID_XDELTAPUSH
//#define ADD_METADATA

// 以下 v4.74
#define UPDATE_20260603 // ログ出力の排他処理化を追加
#define UPDATE_20260605 // 詳細ログファイルのオープン失敗リトライ処理の追加
#define UPDATE_20260606 // ログの書き込み信頼性向上対策
// ログの書き込み用ファイルディスクプリタを新規で開いた後保持して
//　一定時間間隔（１時間間隔）内の書込みは実行し時間を超えた時の書き
//　込みはクローズ再オープンし、書き込み時は、MUTEXで排他する。

// 以下 v4.73
#define UPDATE_20250918 // IDLEポーリングベース間隔(ミリ秒）の調整 ポーリング間隔を nIDLELoop*1000 から nIDLELoop*nIDLEMSLoop でミリ秒単位のポーリング間隔の設定を可能にする正し短すぎるとCPU負荷増大の恐れあり
#define UPDATE_20251105 // POP3に残さない指定時に場合に受任ボックスにファイルが実在していることを確認後にファイル移動するようにした。

// 以下 v4.72
#define UPDATE_20241209 // 外部アカウントでのXOAUTH2での認証方式用にユーザ別認証コード保管ファイル(oauth2.dat)を各アカウントフォルダに設定可能にした
#define UPDATE_20241209_PIPE // 外部アカウントがらの内部アドレスにパイプ設定

// 以下 v4.71
#define UPDATE_20240728 // サブネットマスクの範囲指定の高速化
#define UPDATE_20240827 // FETCH命令で処理トーク解析処理の際のCPU負荷を下げる対策 (UPDATE_20231003 の変更を命令解析の処理にも追加した)
#define UPDATE_20240827A // IMAPフォルダの排他フラグの有効時間をデフォルト30秒に変更した
#define UPDATE_20240830 // FETCH命令でフラグ操作内容が現状と同じ場合はスキップする
//#define UPDATE_20240830A // FETCH命令で処理トーク解析処理の際のCPU負荷を下げる対策(UPDATE_20240827 でスリープする時間を0からPeekCoolTimeとした)
#define UPDATE_20240830B // FETCH命令で処理トーク解析処理の際のCPU負荷を下げる対策

// 以下 v4.70
#define UPDATE_20240121 // FETCH BODY[x.x]<n.x> のの応答で範囲指定が正しくない不具合。
#define UPDATE_20240420 // 暗号化通信方式のセキュリティレベルの設定

// 以下 v4.69
#define UPDATE_20231003 // データ受信時のCPU負荷を下げる対策
#define UPDATE_20231013 // Content-Type:ヘッダに続くデータが改行のみしか無い場合、添付ファイルと認識できない不具合。
#define UPDATE_20231024 // Content-Type:ヘッダに続く次行が改行のみしか無い場合、添付ファイルと認識できない不具合。（上記 UPDATE_20231013 の変更による影響対策)
#define UPDATE_20231025 // Content-Type:ヘッダにnameタグが無いと添付ファイルのエンコードに失敗する不具合。（上記 UPDATE_20231013 の変更による影響対策)
#define UPDATE_20231114 // Content-Type:ヘッダに"message/rfc822"が指定されると構造解析が失敗する不具合。
#define UPDATE_20231117 // FTECH ENVELOPEでFROMの定義可能な上限を増やした。
#define UPDATE_20231119 // Content-Type:ヘッダに"message/rfc822"が指定されその内容がマルチ―パートの構造の場合に解析が失敗する不具合。
#define UPDATE_20231119A // MIMEパートに対する、"HEADER.FIELDS""命令がが失敗する不具合。
#define UPDATE_20231120 // multipart/alternativeパートの2パートの取得に失敗する不具合。
#define UPDATE_20231120A // "Conten-Type: message/rfc822"の子パートの取得に失敗する不具合。
#define UPDATE_20231120B //"HEADER.FIELDS""命令の応答で末尾のカッコが欠落する場合がある不具合。
#define UPDATE_20231121 // HEADER.FIELDS命令の応答で範囲指定が指定できない不具合。
#define UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
#define UPDATE_20231122A // HEADER.FIELDS命令の項目指定が無いとハングする不具合。
#define UPDATE_20231130 // 検索文字列が空欄("")だと検索出来ない不具合。 テスト形式=>". SEARCH CHARSET UTF-8 UNDELETED UNSEEN OR (FROM "") (OR (SUBJECT "") (OR (BODY "") (OR (TO "") (CC ""))))"
#define UPDATE_20231205 // FETCH命令で取得データが空白の場合、0バイトデータとして応答する。
#define UPDATE_20231209 // FETCH命令中に通信切断された場合に大きなサイズのデータ送信の中断を行えるようにした。
#define UPDATE_20231211 // 通信エラーが事前の送信で発生してる場合送信を行わないでセッションを終了するようにした。

// 以下 v4.68
#define UPDATE_20230929 // 256byteを越えるファイル名がMIME-Qエンコードで指定されていると正しいファイルが取得できない

// 以下 v4.67
#define UPDATE_20230523 // OPENSSLライブラリを3.1.xに変更した。
#define UPDATE_20230613 // FETCH ENVELOPEのFETCH命令でTO:CC:ヘッダに複数のアドレスが記載されている場合正しく応答されない不具合
#define UPDATE_20230617 // SEARCH命令でOR指定が含まれると正しく検索結果が出力されない不具合
#define UPDATE_20230618 // FETCH ENVELOPE命令の応答でSender又はReplytoへの回答がNILだと開封確認ダイアログが表示されない(Denbun対策)
#define UPDATE_20230619 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
#define UPDATE_20230620 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
#define UPDATE_20230624 // SEARCH BODY命令で検索速度の向上対策
#define UPDATE_20230624A // SEARCH BODY命令で検索速度の向上対策:検索キーワード側を文字コード別に準備して比較する
#define UPDATE_20230624A2 // SEARCH BODY命令で検索速度の向上対策:検索キーワード側を文字コード別に準備して比較する
#define UPDATE_20230624B // [本文検索が抜ける] SEARCH 変数を重複してカウントしている
#define UPDATE_20230626 //QuotedPrintableの比較の追加
#define UPDATE_20230626A //1行にMIME-Bエンコードの区切りが複数あると、1つ目しか展開できない不具合
#define UPDATE_20230626B //1行にMIME-Bエンコードの区切りが小文字があるとループする不具合
#define UPDATE_20230627 // UPDATE_20230624の有効無効フラグを追加
#define UPDATE_20230627A // UPDATE_20230624のメールデータを事前に一括して読込み検索するフラグを追加
#define UPDATE_20230628 // MIME-Bエンコードの区切りコードが含まれないとハングする
#define UPDATE_20230701 // 検索中にMIME-Bエンコードの行で区切りコードが抜けていると処理が終わらない不具合
#define UPDATE_20230707 // FTECH ENVELOPEでTO,CC,BCCのアドレス定義可能な上限を増やした。
//#define UPDATE_20230706_DEBUG // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加:デバッグ用print文
#define UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
#define UPDATE_20230711 // 検索文字列をダブルクォーテーションで囲うと検索に失敗する
#define UPDATE_20230713 // FETCH命令の文字列の並びに"ALL"が含まれると、"FETCH ALL"として処理してしまう不具合
#define UPDATE_20230714 // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
#define UPDATE_20230714A // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
#define UPDATE_20230714B // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
#define UPDATE_20230714C // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
#define UPDATE_20230714D // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
#define UPDATE_20230714E // Content-Disposition:ヘッダに"filename="の項目のみ出力するオプションを追加　デフォルト filenameのみ
#define UPDATE_20230717 // BulkFetchフラグ有効時にファイルの最終行に改行"\r\n"がないとデータ取得が終わらない
#define UPDATE_20230722 // AUTHENTICATE命令でユーザ名をNULLで返答すると、ログイン成功の応答をしてしまう可能性
#define UPDATE_20230722A // "AUTHENTICATE LOGIN"命令に引数を続けると認証に失敗する不具合
#define ADD_XOAUTH2 // OAUTH2での認証方式を追加
#define ADD_XOAUTH2_A // OAUTH2を加えるとCAPABILITYの表記が認証方式３つまでしか表記できない
#define ADD_XOAUTH2_B // AUTHENTICATE XOAUTH2で改行した場合のトークン入力町を可能に
#define UPDATE_20230801 // FETCH ENVELOPEでファイル読み込みでメモリのオーバーロードが発生してしまう。
#define ADD_XOAUTH2_D // OAUTH2の認可アプリの登録メールアドレスで一致するもののみの許可するようにした。
#define UPDATE_20230821 // "Content-Transfer-Encoding:"ヘッダが"Content-Disposition:"の次にあるとメール本文のエンコーディング情報を正しく返せない
#define UPDATE_20230914 // Reply-to:,Sender:ヘッダの情報取得でメモリリークする場合がある

// 以下 v4.66
#define UPDATE_20220317 // LOGIN命令時の受信するパスワード長を変更した(MAX 64byte)

// 以下 v4.65
#define UPDATE_20211027 // '{'が含まれたトークンを次通信にトークンが続く判定の改善 "<CMD> {size}<CR><LF>"
#define UPDATE_20211210_IPV6PREFIX // IPv6アドレスプレフィックスに対応した。
#define UPDATE_20211213 // 複数のIPv6アドレスが指定されているとサービス起動に失敗する

// 以下 v4.64
#define UPDATE_20210412 // related後の続きでMIMEパートがある場合区切り処理が抜ける対策
#define UPDATE_20210604 // 指定フォルダがフルパスで230byteを超えているとハングする不具合。

// 以下 v4.63
#define UPDATE_20210224 // ADアカウントまたは、WINDOWSアカウント運用時のAUTHENTICATE LOGIN/PLAINでのパスワードファイルの参照を無視する

// 以下 v4.62
//#define UPDATE_20191002 // クライアントに改善認められず：ソート対象日付を作成日付から更新日付に変更した。
#define UPDATE_20201117 // IPv6で接続時の受信ポートが正しく取得できていない不具合

// 以下 v4.61
#define UPDATE_20190426 // 利用する暗号アルゴリズムの組み合わせオプション 
#define UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加

// 以下 v4.60
#define UPDATE_20190207 // 独自アカウントデータベース内のパスワードの暗号化
#define UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
#define UPDATE_20190218 // IP毎の同時接続ロックアウト機能を強化（サンプル時間と拒絶時間の設定を追加）

// 以下 v4.59
#define UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換えた（SSL2を廃止し、TSL1.3を使用可能にする為）

// 以下 v4.58
#define UPDATE_20180817 // IPv6での接続時にロックアウトのカウントが出来るようにした。
#define UPDATE_20180819A // 認証セッション中にロックアウト回数に達したら強制切断する
#define UPDATE_20180819B // 認証成功時にロックアウトカウンタのリセット機能を追加
#define UPDATE_20180820 // 暗号認証でロックアウトの処理が出来ない
#define UPDATE_20180820A // SASLにPLAIN方式を追加
#define UPDATE_20180827 //ファイルオープン中でサービス停止時が行われたときにハングしない対策

// 以下 v4.57
#define UPDATE_20171230 // EXAMINE命令のフォルダ名に末尾に半角スペースが含まれると、フォルダ移動が永久ループする不具合
#define UPDATE_20171230A // Quoted-Printableのメールの検索への対応
#define UPDATE_20180107 // SEARCH命令がCHARSETのみしかないとハングする不具合
//#define UPDATE_20180109 // BODY.PEEK[1.1] の応答が出来ない [追加不要]
#define UPDATE_20180109A //Content-Transfer-Encoding:ヘッダ情報が空白の場合の対策
#define UPDATE_20180110 // rfc822構造添付ファイルに複雑なMIME構造の生成に失敗する場合があった

// 以下 v4.56
#define UPDATE_20171211A // 使用可能な暗号化通信方式のバージョンを規定するオプション 0x1:SSL2禁止 0x2:SSL3禁止 0x4:TSL1禁止 0x8:TSL1.1禁止 0x10:TSL1.2禁止

// 以下 v4.55
#define UPDATE_20170619 // 中間証明書の繋がりが反映されていなかった

// 以下 v4.54
#define UPDATE_20170207 // LDAP接続失敗時のリトライ処理の修正
#define UPDATE_20170223 // フォルダ削除でのメール一括削除オプション追加

// 以下 v4.53
#define UPDATE_20161221 // セッションクローズ処理でIDLEフラグポインタが不定値で判定している不具合
#define UPDATE_20170114 // UID値書き込み用のスペースに空きが無いときロックする不具合
#define UPDATE_20170114A // INBOXフォルダへ移動失敗時の対策を追加

// 以下 v4.52
#define UPDATE_20160823 // APPEND命令で存在しない追加フォルダを指定すると処理が終了できなくなる不具合
#define UPDATE_20160830 // 接続拒絶用テーブルの指定にホワイトリストの指定を追加した。"書式 <IP-Addr.><TAB>true"
#define UPDATE_20161006 // フォルダ選択時にインデックス情報(imap.$dx)が残っているとハンドルのクローズが抜ける
#define UPDATE_20161013 // ログイン成功時にルートフォルダが存在しない場合、自動作成を試みるようにした。

// 以下 v4.51
////#define UPDATE_20151226 // 当分封印：IDLE処理の修正 (同一フォルダにアクセス中のセッションに変化が伝わらない不具合)
#define UPDATE_20160113 // 認証時のロックアウト機能を追加
#define UPDATE_20160114 // 接続拒絶用テーブルの指定に有効時間を設定可能にした。
#define UPDATE_20160128 // 存在しないアカウントへの認証失敗ロックアウト管理用フォルダを作成

// 以下 v4.50
#define UPDATE_20151028 // セッションスレッドの作成に失敗した時の対策
#define UPDATE_20151103 // Openssl 1.0.2dに変更した
#define UPDATE_20151111 // ログインパスワードにダブルクォーテーションか円マークが含まれていると失敗する
// PW例 "te\"st\"01"
//#define UPDATE_20151115 // 速度変わらず：SELECT時にメール一覧をキャッシュするためのポインタ
#define UPDATE_20151122 // 特定接続元IPからの接続拒絶用テーブル追加(rejectimapip.dat)隠しオプション
#define UPDATE_20151122A // STARTTLSで暗号化通信に切り替わらない不具合
#define UPDATE_20151125  // SSL証明書と秘密鍵が指定されているときCAPABILITY命令にSTARTTLSを含めるようにした。
#define UPDATE_20151126 // リッスンＩＰ毎に証明書を選択できる対策(レジストリで指定)
#define UPDATE_20151127 // リッスンＩＰ毎に証明書を選択できる対策(sslbyip.datファイルで指定)
#define ADD_IDCACHE     // Windows,AD,LDAP連携で問合せキャッシュ機能を追加。(問合せ負荷の軽減策)
#define UPDATE_20151215 // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
#define UPDATE_20151215A // クリティカルセッションフラグ：高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
#define UPDATE_20151217 // 範囲指定に0を指定するとワイルドカード処理されてしまう不具合
#define UPDATE_20151218 // 範囲指定の*のとき処理の修正
#define UPDATE_20151220 // SEARCH命令でハングする可能性(". SEARCH UID" に続くデータがないときハングする)
//#define UPDATE_20151225 // 試作（応答時のファイル数がカウントできないので不採用）：UID値によるファイル範囲指定のチューニング
#define UPDATE_20151225A // FETCH命令のALL,FULL,FASTが小文字だと処理できない不具合

// 以下 v4.49 
#define UPDATE_20150616  // BODYSTRUCTUREのマルチパート構造の応答時に半角スペースの区切りが挿入されない箇所があった
#define UPDATE_20150617  // HEADER(Content-Type:)構造が１行内に複数の情報がセミコロンで区切られていると正しく抽出できない不具合
#define UPDATE_20150617A // Content-Type: multipart/alternative の箇所の構造を出力できない不具合
#define UPDATE_20150617B // 応答に"NIL"追加
#define UPDATE_20150617C // HEADER(Content-Disposition:)構造にfilename*N*=が複数行にわたり記載されていると正しく抽出できない不具合
#define UPDATE_20150618  // HEADER(Content-Disposition:)構造のattachmentの続きに半角スペースだけあると正しく抽出できない不具合
#define UPDATE_20150618A // HEADER(Content-Type:)構造で改行されずにboundaryが記述されていると正しく抽出できない不具合
#define UPDATE_20150618B // HEADER(Content-Type:)構造でnameに続くファイル名が改行後に記述されている正しく抽出できない不具合
#define UPDATE_20150618C // HEADER(Content-Type:)構造でmultipart/alternativeの入れ子構造が正しく抽出できない不具合
#define UPDATE_20150620  // HEADER(Content-Type:)構造でname=""の場合に正しく抽出できない不具合
#define UPDATE_20150620A // HEADER(Content-Type:)構造でboundaryの文字列の後ろに空白がある場合に正しく抽出できない不具合
#define UPDATE_20150620B // 多重入れ子構造がある場合に正しく抽出できない不具合
#define UPDATE_20150623 // 一括でのBODY情報取得時に2件目以降の応答内容がが欠けてしまう不具合。
#define UPDATE_20150623A // BODY[HEADER]<x.n>で指定したヘッダが出力されない不具合。
#define UPDATE_20150623B // BODY[n.MIME]で指定した情報が出力されない不具合。
#define UPDATE_20150624 // SEARCH命令で検索対象NOを複数羅列するとハングする不具合
#define UPDATE_20150625 // SEARCH命令で検索対象NOを複数羅列すると正しい結果が得られない不具合
#define UPDATE_20150626 // SEARCH命令で検索対象NOにNOT指定を行うと正しい結果が得られない不具合
#define UPDATE_20150626A // SEARCH命令で検索対象NOをUID値で指定を行うと正しい結果が得られない不具合
#define UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
#define UPDATE_20150630A // FETCH N BODY[1]でboundary=がダブルクォテーションで囲まれていないと正しく読み込めない不具合
#define UPDATE_20150630B // FETCH N BODYSTRUCTでContent-Transfer-Encoding: base64の時の応答結果が"7bit"になってしまう場合がある不具合
#define UPDATE_20150630C // MESSAGE/RFC822の構造出力への対応
#define UPDATE_20150701 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)
#define UPDATE_20150701A // 本文が0バイトの応答にNILを出力しないようにした。
#define UPDATE_20150702 // QMAIL3で添付付きメールがFETCHエラーにならないようにする対策。(UID付きFETCHの応答でUID値を最後に出力していたのを最初に出力するように修正した。)
#define UPDATE_20150702A // BODY[n.n]のように深い階層にデータがないと上位層のデータを応答してしまう不具合。
#define UPDATE_20150703  // BODY[]のときMIME構造があると出力サイズ値と実データが一致しない場合があった。
#define UPDATE_20150703A  // BODY[HEADER]のとき応答の先頭に余分なスペースを出力してしまっていた。
#define UPDATE_20150705 // 複数の命令が１度に受信された時、応答結果に無効なデータが含まれることがある
#define UPDATE_20150706 // Content-Type: 行に boundaryが含まれていると入れ子のbody[n.n]等が正しく抽出できない
#define UPDATE_20150707 // BODY[]<x.n>で指定した応答でBODY[]<x>で応答していない不具合。
#define UPDATE_20150708 //  MIME構造で(multipart(text,multipart(html))のような構造で順番道理の結果が返せない不具合。
#define UPDATE_20150708A // FETCH n ENVELOPEでメールアドレス内に不正な文字がある対策
#define UPDATE_20150708B // MESSAGE/RFC822の構造出力結果がバッファオーバフローする可能性のある不具合。
#define UPDATE_20150709 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
#define UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
#define UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
#define UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
#define UPDATE_20150712A // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
#define UPDATE_20150712B // MIMEヘッダが空の構造の時正しい構造が生成できない不具合。
#define UPDATE_20150712C // Content-TypeにPGPが含まれている時正しい構造が生成できない不具合。
#define UPDATE_20150712D // MIMEパートの終わり判定が抜ける対策
#define UPDATE_20150712E // MIMEパートの区切り処理が抜ける対策
#define UPDATE_20150712F // MIMEパートの区切り処理が抜ける対策

// 以下 v4.48
#define UPDATE_20150601 // MS-OFFICEファイルのMIMEタイプが長すぎて途中で切られてしまう
#define UPDATE_20150604 // 添付ファイルの名称が複数行にまたがっている場合の処理
#define UPDATE_20150606 // 先頭にORがある時、検索結果が正しく出力されない
#define UPDATE_20150606A // 先頭にORがある時、検索結果が正しく出力されない
#define UPDATE_20150606B // 検索条件が長いと途中で切ってしまう不具合(Becky!でのリモート検索で正常に機能しない)
#define UPDATE_20150607 // 検索階層カウンタを追加
#define UPDATE_20150608 // 検索時の日付をUTC日付かローカル日付かに変更できるオプションを追加した
#define UPDATE_20150609 // フォルダへのコピー時に受信日付がコピー日付になってしまった不具合 TRUE:ローカル日付 FALSE:UTC日付
#define UPDATE_20150611 // SELCET命令のフォルダ名に末尾に半角スペースが含まれると、フォルダ移動が永久ループする不具合
#define UPDATE_20150612 // SEARCH命令の構造解析見直し
#define UPDATE_20150615 // 検索条件でHEADER <KEY>をダブルクォーテーション囲まれていると失敗する
#define UPDATE_20150615A // BODYSTRUCTUREでヘッダ構造に誤った解析結果を出力する場合があった

// 以下 v4.47
#define UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
#define UPDATE_20141221 // セッション内でのメッセージ送信の排他処理
#define UPDATE_20141222 // 同報セッション情報の保管先を接続ウント単位に保存する対策
#define UPDATE_20141225 // フォルダ切り替えで排他フラグがリセットさせる対策
#define UPDATE_20150107 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策(IDLE/NOOP)

// 以下 v4.46
#define UPDATE_20141115 // 独自アカウント運用時で最大文字数を128byteまで拡張可能にする対策をした。
#define UPDATE_20141118 // カラのフォルダに対しSTORE命令実行すると一部のメモリ開放が抜ける不具合。

// 以下 v4.45
#define UPDATE_20141017 // IMAPフォルダ指定が空欄の場合はフラグ集計しない
#define UPDATE_20141018 // IDLEスレッドの起動に失敗しハングする可能性の対策
#define UPDATE_20141025 // DONEコマンド受信時にIDLEスレッドのハンドル値をリセットしてメモリ開放抜け
//#define UPDATE_20141025A // IDLEコマンドデフォルトで無効にした
#define UPDATE_20141027 // 未ログイン時のセッションでIDLE命令が機能してしまう不具合。

// 以下 v4.44
#define UPDATE_20140606 // NOOPポーリング結果でフォルダ内に変化があった場合のみ結果応答をしデフォルトで有効にする
//#define UPDATE_20140610 // IDLEコマンド実行中の無通信タイムアウト時間をデフォルトで30分にした。
#define UPDATE_20140610A // 実行中の受信側無通信タイムアウト時間をデフォルトで30分に変更した。
#define UPDATE_20140620 // "UID SEARCH"での番号指定での誤った応答結果を返してしまう不具合。
#define UPDATE_20140620A // SEARCH命令に続くUIDトークンの引数が不足しているとハングする不具合
#define UPDATE_20140723 // 64bit版での送信セッション中断でハングする

// 以下 v4.43
#define UPDATE_20140528 // IDLEコマンド実装
#define UPDATE_20140530 // NOOPによるポーリング結果送信有無

// 以下 v4.42
#define UPDATE_20140523 // APPEND元のフォルダ名に空白が含まれていると実行が失敗する
#define UPDATE_20140523A // APPEND実行時のフラグが未指定の時ハングする

// 以下 v4.41
#define UPDATE_20140522 // IMAPでフォルダを選択した時、排他処理フラグが同期されずに以降の処理をロックさせてしまう場合がある

// 以下 v4.40
#define UPDATE_20130625 // $Forwarded フラグを利用可能にしたのとThunderbardでAnsweredフラグがセットされない対策

// 以下 v4.39
#define UPDATE_20130531 // フォルダ名に半角スペースがあると処理が失敗することがある不具合
#define UPDATE_20130529 // フォルダ作成直後に検査用一時ファイルが開放されずにフォルダが削除できなくなる不具合。
#define UPDATE_20130528 // ログイン時に作成済みのINBOXフォルダがあると、無駄に作成リトライ待ちが発生する不具合

// 以下 v4.38
#define UPDATE_20110909 // ML名+ドメイン名が53byteを超えるとハングする

// 以下 v4.37
#define UPDATE_20110525 // IPv6でのIP単位での接続設定のとき内部ドメイン宛の接続を拒否してしまう不具合

//#define SEARCH_NO_RESS_TEST // SEARCHで使用不可のオプションでNOを返すテスト
// 以下 v4.36
#define UPDATE_20110414 // NOOPタイマー（一定時間新着がないNOOPでセッション強制切断する対策（負荷軽減対策）
#define UPDATE_20110406A// 他の命令でフォルダ処理中はNOOPでフォルダチェックしない
#define UPDATE_20110406 // フォルダ削除時にrid,store,fetch処理用のインデックスファイルを削除する。
#define UPDATE_20110405B // インデックス用ファイルに一覧書込みが完全に完了させる対策。
#define UPDATE_20110405A // UID値の排他処理強化
#define UPDATE_20110405 // LOGIN,AUTHENTICATED命令でUID値取得をしないようにした。（排他処理対策）
#define UPDATE_20110404B // サービス再起動前のロックファイルを削除する
#define UPDATE_20110404A // 存在しないフォルダを選択すると排他処理で無限ループする不具合
#define UPDATE_20110404 // 選択フォルダ単位で排他する対策
#define UPDATE_20110326 // サービス起動時最初のフォルダ選択について判定し最初なら回復処理を実施する
#define UPDATE_20110325A // 先頭カラムにチルダのあるメールファイルの回復処理
#define UPDATE_20110325 // 回復機能をデフォルトに変更

// 以下 v4.35
#define UPDATE_20110304 // v4.34(フラグ変更の為の拡張子変更)使用している場合のバージョン差替え時の拡張子修復対策。
#define UPDATE_20110303A // POP3使用しない設定でIMAPフォルダへのメール移動処理の改修
#define UPDATE_20110303 // UID値が0x7FFFFFFF(2147483647)を超えるとマイナス値で表示されてしまう不具合。
#define UPDATE_20110302B // ログイン時のファイル移行でループする可能性を除去する対策。
#define UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
#define UPDATE_20110302 // リカバリ処理でファイル名が正しく変更されるまで待つ対策
#define UPDATE_20110301C // ロックファイルの開放が完全に確認できるまで待つ対策
#define UPDATE_20110301B // サービス停止で残ったユーザ毎のロックファイルをサービス再起動後に一旦削除する対策
#define UPDATE_20110301A // FETCHコマンドでフラグ変更時に拡張子"~SG"のファイルが残ってしまうタイミングが無いようにする対策
#define UPDATE_20110301 // STOREコマンドでフラグ変更時に拡張子"~SG"のファイルが残ってしまうタイミングが無いようにする対策
#define UPDATE_20110228B // 誤った構造のCOPYコマンドを送るとハングする
#define UPDATE_20110228A // 誤った構造のSTOREコマンドを送るとハングする
#define UPDATE_20110228 // サービス停止のタイミングでフラグ変更中のファイルの復旧対策
#define UPDATE_20110222 //UPDATE_20110202でパフォーマンス改善の対策。

// 以下 v4.34
#define UPDATE_20110208 // フォルダ作成が完了するまでウエイトする。
#define UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
#define UPDATE_20110203A //フラグ変更中に新着の検出が発生したとき最後のUIDを正しく取得できないタイミングがある不具合
#define UPDATE_20110203 // UID回復処理で該当フォルダにメールが無いときUID値が初期化されてしまう不具合
#define UPDATE_20110202 //IMAPフォルダ内の連番構造で重複ファイル名が発生した場合の回復機能の追加
#define UPDATE_20110126 // １カラム目のピリオドは出力しない対策
#define UPDATE_20110125 // 誤った構造のSTOREコマンドを送るとハングする
#define UPDATE_20110124 // STORE N FLAGSでフラグがリセットされる不具合
#define UPDATE_20110124A // Subject:のFetchで途中で文字が切れるないように変数領域を増やした。

// 以下 v4.33 (ソリトン様顧客でのトラブル対策)
#define UPDATE_20101229 // ユーザ名＋ドメイン付きの長さも３０バイトまでで切られてしまう不具合
#define UPDATE_20101224A // フラグを変更したファイルがファイル一覧取得時に反映されないようにする対策
//#define UPDATE_20101224 // SOCKET送信でのERROR=183に対する対策
#define UPDATE_20101221 // UID値ファイルのクリティカルセクション化

// 以下 v4.32 (CatchMe@Mailとの相性を良くするための対策)
//#define UPDATE_20091129 // ENVELOPE応答で、日付に曜日が含まれないとCatchMe@Mailが正常に一覧表示しない
#define UPDATE_20091129A // RFC822.HEADER.LINES/RFC822.HEADER.LINESコマンドの追加 CatchMe@Mailで添付きメールの表示が失敗する
//#define UPDATE_20091129B // ファイル名のバイト数表示 CatchMe@Mailで添付きメールの表示が失敗する
#define UPDATE_20091129C // MIME区切りの")("のスペースをいらないようにした。 CatchMe@Mailで添付きメールの表示が失敗する
#define UPDATE_20091129D // BODY.PEEK応答時にBODYに変換。CatchMe@Mailで添付きメールの表示が失敗する
#define UPDATE_20091129E // "filename*N*"時のファイル名生成の手直し CatchMe@Mailで添付きメールの表示が失敗する
#define UPDATE_20091129F // BODY[N]<start.size>処理の手直し CatchMe@Mailで添付きメールの表示が失敗する
#define UPDATE_20091129G // MIMEパート取得でファイルクローズ抜け CatchMe@Mailで添付きメールの表示が失敗する
#define UPDATE_20091129H // MESSAGE/RFC822の添付表示 CatchMe@Mailで添付きメールの表示が失敗する
#define UPDATE_20091129I // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
#define UPDATE_20091201  // SEARCH命令に余分なスペースがある場合の対策
#define UPDATE_20091203 // SEARCHの番号指定で結果フラグが初期化されていなかった。

// 以下 v4.31
#define UPDATE_20091224 // 前バージョン環境変数%systemroot%の対策ロジックのミス

// 以下 v4.30
#define UPDATE_20090903 // メール作業領域で環境変数%systemroot%があるとドライブチェックが正しく出来ない
#define UPDATE_20091013 // mLDAPSchemaID 
#define UPDATE_20091014 // 深い階層のアカウントが読めるように対策
#define UPDATE_20091014A // ドメイン無しアカウントでホームディレクトリ参照時ハングする可能性がある
#define UPDATE_20091014C // LDAPへのアカウント問合せ処理の高速化
#define UPDATE_20091014D // LDAPへのアカウント認証時、ドメインに含まれないアカウントの認証も成功してしまう不具合
#define UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
#define UPDATE_20091101 // HOMEディレクトリタイプで無いならホームフォルダの検索をしないようにしAD連携時の処理速度の向上を図った。
#define UPDATE_20091103 // LDAP連携専用のリトライ回数指定の変数に変更
#define UPDATE_20091110 // POP3メールボックスフォルダ位置をINBOXに変更するオプションを追加（"$sharepop3"をメールボックスに設定すると有効）
#define UPDATE_20091116 // CAPABILITY 命令で応答する認証方法を設定変更可能にした。

// 以下 v4.29
#define UPDATE_20090610 // %HOME%が設定されているとまともに動かない
#ifdef USE_SSL
#define USE_STARTTLS
#endif
// 以下 v4.28
#define UPDATE_20090326 // SEARCH命令でヘッダの文字列がエンコードされているとデコード範囲以降の文字列の検索が欠落してしまっていた。
#define UPDATE_20090325 // 複雑なSEARCH命令の解読でハングすることがある。
#define UPDATE_20090325A // SEARCH命令で指定文字列の大文字小文字の区別がされていた。
#define UPDATE_20090325B // SEARCH命令で複雑な入れ子の命令が処理されない。
#define UPDATE_20090325C // メッセージのBODYパート数の上限を超えた場合ハングする
//#define UPDATE_20090322 // ログイン時のロックファイルのクリティカルセクション化
//#define UPDATE_20090320 // フォルダ未選択でUIDリクエストを受けると以降のリクエストがロックしてしまう
//#define UPDATE_20090320A // 別セッションのリッスン状態初期化で設定エラーが発生するとサービスが停止してしまうす。
// 以下 v4.27
#define UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
// 以下 v4.26
//#define UPDATE_20090113 // SSL終了時の処理修正（SSL_shutdown抜け）
#define UPDATE_20090108 // SSL用中間証明書
// 以下 v4.25
#define UPDATE_20071205 // 外部メールアドレス長が256Byteのときの対策
// 以下 v4.24
#define UPDATE_20071121 //IPv6で全てのIPアドレスに応答のとき停止が出来ない
// 以下 v4.23
//#define UPDATE_20071113 // 属性が隠しファイルでも処理を可能にする
#define UPDATE_20071016 // LDAPサーバーへの接続リトライ
// 以下 v4.22
#define UPDATE_20070620 // ドメインごとにDNを設定可能にする
// 以下 v4.21
#define UPDATE_20070521 // OSの予約語対策
// 以下 v4.20
#define UPDATE_20070425 // MSCSのスタンバイ側に対応
#define UPDATE_20070405 // イベントログデータベースを追加。
#define UPDATE_20061217 // 特殊なトークン".. *"でフォルダ階層指定を行うと処理から抜けられない不具合
//#define UPDATE_20061031 // 開放されていないメモリをリセットしてしまう可能性
#define UPDATE_20061011 // ソケット処理のselect()がエラーとして返えった後、以降のソケットのリッスンが出来なくなる可能性の対策。
#define UPDATE_20060807 // 選択中のフォルダのuid 値をINBOXフォルダのUIDと置き換わる可能性のある不具合
#define UPDATE_20060804 // uid 値の最終値が不正な値を作成してしまう可能性のある不具合
#define UPDATE_20060606 // ドメインコントローラへの接続リトライ
#define UPDATE_20060605 // SSL_acceptの応答コードが０の時に正常としていた不具合
#define UPDATE_20060515 // SOCKET関連のメモリ開放抜け
#define UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
#define UPDATE_20060418 // カッコつきの処理 (v4.11)
#define UPDATE_20060329 // SSLの初期化に失敗すると以降でハングする
#define UPDATE_20060324 // Thunderbirdで添付を含むSSL送信で問題をおこす。
#define UPDATE_20060322 // SSLの初期化に失敗すると以降でハングする
#define UPDATE_20060315 // 隠すIPアドレス
#define UPDATE_20060303 // NOOPで状態応答
#define UPDATE_20060117 // 強制切断でCPUが１００％状態になる
#define UPDATE_20060116 // ユーザルートより上位のフォルダにアクセスできる脆弱性３
#define UPDATE_20050531_A  // ユーザルートより上位のフォルダにアクセスできる脆弱性その２
#define UPDATE_20050531 // CRETAE命令にバッファフローの脆弱性
#define UPDATE_20050530 // ユーザルートより上位のフォルダにアクセスできる脆弱性
#define UPDATE_20050518 // SSLデータ読み込み時のタイムアウトチェックの処理修正
//#define UPDATE_20050514 // 接続IP別に対応したドメインのみに応答する
//#define UPDATE_20040707
#define LINK_FOLDER
#define UPDATE_20040428
#define UPDATE_20040202
#define Y2038_BUG 1

#define WAIT_TIME 0 //10

#define MAX_ATTRIBUTE 100 //20
#define MSGID_MAX_ATTRIBUTE 100
#define MAX_FILE 50
#define SLEEP_TIME 10
#define SLEEP_TIME_CLOSE   SLEEP_TIME //10
#define SLEEP_TIME_VIEW    SLEEP_TIME //20
#define SLEEP_TIME_ADD     SLEEP_TIME //30
#define SLEEP_TIME_DELETE  SLEEP_TIME //40
#define SLEEP_TIME_UPDATE  SLEEP_TIME //50
#define SLEEP_TIME_TRANS   SLEEP_TIME //60
#define SLEEP_TIME_CHECK   SLEEP_TIME //70
#define SLEEP_TIME_TIMEOUT SLEEP_TIME //100

#define WAIT_TIME 0 //10
#ifdef E_POST
  #define REGTOFILE
  #define CLUSTERING
  //#define FREE_LICENCE // フリーライセンス無効
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
#include <mswsock.h>
#ifdef VC6
#include "..\ipv6-src\inc\ws2ip6.h"
#else
#include <ws2tcpip.h>
#define AI_V4MAPPED     1
#define AI_ALL          2
#define AI_ADDRCONFIG   4
#define AI_DEFAULT      (AI_V4MAPPED | AI_ADDRCONFIG)
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

#include "imap4type.h"
#include "profile.h"

#define IMAP4_LOCKFILE       TEXT("$LockFile")
#define IMAP4_LOCKFILE_NAME  TEXT("\\") IMAP4_LOCKFILE
#define IMAP4_FIND           TEXT("\\*.MSG")
#define IMAP4_UID_INDEX      "uid.idx"
#define POP3_SHARE           TEXT("$sharepop3")
#define IMAP4_OFF_POP3       TEXT("$offpop3")
#define IMAP4_OFF_IMAP4      TEXT("$offimap4")
#define IMAP4_SHARED_FILE    TEXT("$Shared")
#define IMAP4_SHARED_NAME    "#shared/"


#define MAXIMAPFOLDER 128
#define MAILTMPLEN 1024		// size of a temporary buffer
#define NETMAXMBX (MAILTMPLEN/4)

#define TRACE_MODE 1
#define TMQ_ON 1
#define ACCEPT_LOG_LEVEL3 1
//#define TEST_MODE 1
//#define APOP 1
//#define ADD_AUTH 1
#define TZNAME_MAX 50

/// It limit a relay pop3 receiver. spare
#define DEFAULT_PORT      143
#define ADCACHE        "adimap"
#ifdef E_POST
#ifdef FOR_BRIDGEGATE
#define SOFTNAME       "IMAP4rev1 Server for BridgeGate" 
#else
#define SOFTNAME       "IMAP4rev1 Server"
#endif
#else
#ifdef V4
#define SOFTNAME       "IMAP4rev1 Server @Solomon" 
#else
#ifdef HOME_VERSION
#define SOFTNAME       "IMAP4rev1 Server @Home" 
#else
#define SOFTNAME       "IMAP4rev1 Server"
#endif
#endif
#endif

#define LDAPOPTION_REG  "SOFTWARE\\EPOST\\LDAPOPTION"
#define LDAPOPTION_BEGINLOW  "BeginLow"
#define LDAPOPTION_BEGINHIGH "BeginHigh"
#define LDAPOPTION_LIMITKEY  "LimitKey"

#ifdef E_POST
  #define APOP              1
  #define TRADEMARK         "E-POST " //"Premix "
  #define IMAP4_SERVICE     "EPSTIMAP4S"
  #define POP3_SERVICE      "EPSTPOP3S"
  #define SOFT_REG          "SOFTWARE\\EMWAC\\IMS"
  #define SOFT_ALIASES_REG  "SOFTWARE\\EMWAC\\IMS\\Aliases" //"SOFTWARE\\EPOST\\IMS\\Aliases"
  #define SOFT_LISTS_REG    "SOFTWARE\\EMWAC\\IMS\\Lists"   //"SOFTWARE\\EPOST\\IMS\\Lists"
  #define DOMAIN_REG        "SOFTWARE\\EPOST\\IMS\\Domain"
  #define DOMAIN_SMTPIP     "SOFTWARE\\EPOST\\IMS\\Domain\\SmtpIP"
  #define DOMAIN_POP3IP     "SOFTWARE\\EPOST\\IMS\\Domain\\Pop3IP"
  #define DOMAIN_IMAP4IP    "SOFTWARE\\EPOST\\IMS\\Domain\\Imap4IP"
  #define DOMAIN_IMAP4SESS  "SOFTWARE\\EPOST\\IMS\\Domain\\Imap4Session"
  #define DOMAIN_FOLDER     "SOFTWARE\\EPOST\\IMS\\Domain\\Folder"
  #define DOMAIN_BASEDN     "SOFTWARE\\EPOST\\IMS\\Domain\\BDN"
  #define MAIL_SPOOL        "%SystemRoot%\\SYSTEM32\\E-POST\\MAIL"
#else
  #define APOP            1
#ifdef V3
  #define TRADEMARK        "SPA-PRO "
  #define IMAP4_SERVICE    "SPAIMAP4S-PRO"
  #define POP3_SERVICE     "SPAPOP3S-PRO"
#else
  #define TRADEMARK        "SPA "
  #define IMAP4_SERVICE    "SPAIMAP4S"
  #define POP3_SERVICE     "SPAPOP3S"
#endif
  #define SOFT_REG         "SOFTWARE\\EMWAC\\IMS"
  #define SOFT_ALIASES_REG "SOFTWARE\\EMWAC\\IMS\\Aliases"
  #define SOFT_LISTS_REG   "SOFTWARE\\EMWAC\\IMS\\Lists"
#ifdef V3
  #define DOMAIN_REG      "SOFTWARE\\SPA-Pro\\IMS\\Domain"
  #define DOMAIN_SMTPIP   "SOFTWARE\\SPA-Pro\\IMS\\Domain\\SmtpIP"
  #define DOMAIN_POP3IP   "SOFTWARE\\SPA-Pro\\IMS\\Domain\\Pop3IP"
  #define DOMAIN_IMAP4IP  "SOFTWARE\\SPA-Pro\\IMS\\Domain\\Imap4IP"
  #define DOMAIN_IMAP4SESS "SOFTWARE\\SPA-Pro\\IMS\\Domain\\Imap4Session"
  #define DOMAIN_FOLDER   "SOFTWARE\\SPA-Pro\\IMS\\Domain\\Folder"
#else
  #define DOMAIN_REG      "SOFTWARE\\SPA\\IMS\\Domain"
  #define DOMAIN_SMTPIP   "SOFTWARE\\SPA\\IMS\\Domain\\SmtpIP"
  #define DOMAIN_POP3IP   "SOFTWARE\\SPA\\IMS\\Domain\\Pop3IP"
  #define DOMAIN_IMAP4IP  "SOFTWARE\\SPA\\IMS\\Domain\\Imap4IP"
  #define DOMAIN_IMAP4SESS "SOFTWARE\\SPA\\IMS\\Domain\\Imap4Session"
  #define DOMAIN_FOLDER   "SOFTWARE\\SPA\\IMS\\Domain\\Folder"
#endif
  #define MAIL_SPOOL        "%SystemRoot%\\SYSTEM32\\SPA\\MAIL"
#endif

#define MAIL_BOX           "%HOME%\\INETMAIL\\INBOX"
#define EVENT_REG          "SYSTEM\\CurrentControlSet\\Services\\Eventlog\\Application\\"
#define SYSTEM_REG         "SYSTEM\\CurrentControlSet\\Services\\" IMAP4_SERVICE
#define SYSTEM_REG_POP3    "SYSTEM\\CurrentControlSet\\Services\\" POP3_SERVICE
#define SYSTEM_PARAM_REG    SYSTEM_REG "\\Parameters"
#define IMAP4_NAME         TRADEMARK SOFTNAME
#ifdef BETA
#define IMAP4_DEBUG_MESS   IMAP4_NAME " Beta%s %s %s"
#else
#define IMAP4_DEBUG_MESS   IMAP4_NAME " %s %s %s"
#endif


#define IMAP4_GOOD_RESPONSE     "OK"
#define IMAP4_RESULT_RESPONSE   "NO"
#define IMAP4_BADTOKEN_RESPONSE "BAD"
#define IMAP4_PREAUTH_RESPONSE  "PREAUTH"
#define IMAP4_BYE_RESPONSE      "BYE"

// 特殊アトム "NIL"
#define NIL                 NULL

// STARTTLSコマンド
#define IMAP4_STARTTLS      "STARTTLS"
#define IMAP4_OK_STARTTLS    IMAP4_GOOD_RESPONSE " Begin TLS negotiation now"
#define IMAP4_BAD_STARTTLS    IMAP4_BADTOKEN_RESPONSE " Command not permitted when TLS active"

// 状態共通コマンド
#define IMAP4_CAPABILITY    "CAPABILITY"
#define IMAP4_NOOP          "NOOP"
#define IMAP4_LOGOUT        "LOGOUT"

// 非認証状態コマンド
#define IMAP4_AUTHENTICATE  "AUTHENTICATE"
#define IMAP4_LOGIN         "LOGIN"

// 認証済状態コマンド
#define IMAP4_SELECT        "SELECT"      //フォルダ選択(Read/Write)
#define IMAP4_EXAMINE       "EXAMINE"     //フォルダ選択(Read only)
#define IMAP4_CREATE        "CREATE"      //フォルダ作成
#define IMAP4_DELETE        "DELETE"      //フォルダ削除
#define IMAP4_RENAME        "RENAME"      //フォルダ名変更
#define IMAP4_SUBSCRIBE     "SUBSCRIBE"   //購読一覧へ追加
#define IMAP4_UNSUBSCRIBE   "UNSUBSCRIBE" //購読一覧から削除
#define IMAP4_LIST          "LIST"        //購読一覧を参照（全体）
#define IMAP4_LSUB          "LSUB"        //購読一覧を参照（指定条件フォルダ下）
#define IMAP4_STATUS        "STATUS"      //指定フォルダの状態
#define IMAP4_APPEND        "APPEND"      //指定フォルダ下へメッセージ追加
#define IMAP4_NAMESPACE     "NAMESPACE"   //NAMESPACE応答
#ifdef UPDATE_20140528 // IDLEコマンド実装
#define IMAP4_IDLE          "IDLE"        //IDLE開始
#define IMAP4_DONE          "DONE"        //IDLE解除
#endif
#ifdef ADD_ID_XDELTAPUSH
#define IMAP4_ID            "ID" 
#endif
#ifdef ADD_METADATA
#define IMAP4_SETMETADATA   "SETMETADATA" 
#define IMAP4_GETMETADATA   "GETMETADATA"
#endif

// 選択済状態コマンド
#define IMAP4_CHECK         "CHECK"       //NOOPと同義
#define IMAP4_CLOSE         "CLOSE"       //選択中のフォルダの\Deleted フラグ分のメッセージ削除後、認証状態へ遷移
#define IMAP4_EXPUNGE       "EXPUNGE"     //選択中のフォルダの\Deleted フラグ分のメッセージ削除後、遷移しない
#define IMAP4_SEARCH        "SEARCH"      //指定中フォルダ内のメッセージ条件式検索
#define IMAP4_FETCH         "FETCH"       //指定中フォルダ内のメッセージ関連情報
#define IMAP4_STORE         "STORE"       //指定中フォルダ内のメッセージフラグ置換え
#define IMAP4_COPY          "COPY"        //指定中フォルダ内のメッセージを指定先フォルダへ複写
#define IMAP4_UID           "UID"         //指定中フォルダ内のメッセージ指定をユニークな番号(ファイル名等）で指示する指定

#define IMAP4_START_FAIL   IMAP4_RESULT_RESPONSE  " %s\r\n"
#define IMAP4_START_MESS   "* " IMAP4_GOOD_RESPONSE " " IMAP4_NAME " " VERSION " Ready. %s\r\n" // %s\r\n"
//#define IMAP4_START_MESS   "* " IMAP4_GOOD_RESPONSE " [CAPABILITY IMAP4rev1 SASL-IR LOGIN-REFERRALS ID ENABLE IDLE LITERAL+ STARTTLS AUTH=PLAIN] " IMAP4_NAME " " VERSION " Ready. %s\r\n" // %s\r\n"
#define IMAP4_START_MESS_HIDE   "* " IMAP4_GOOD_RESPONSE " Ready.\r\n" // %s\r\n"


#endif