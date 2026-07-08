------------------------------------------------------------------
SPA-Pro IMAP4 Server (βバージョン)v0.01       (Update 2002.11.21)
------------------------------------------------------------------
[はじめに]
-------------------------------------------------------------------------
  "SPA-Pro IMAP4 Server"では、以下のコマンドに対応しています。
  -----------------------------------------------------------------------
  IMAP4 Serverは、RFC 2060において定義されているIMAP4rev1プロトコルを実行します。 
  このバージョンにてサポートされるIMAP4コマンドは： 
  -----------------------------------------------------------------------
  CAPABILITY
  NOOP
  LOGOUT
  AUTHENTICATE (CRAM-MD5 or LOGIN)
  LOGIN
  SELECT
  EXAMINE
  CREATE
  DELETE
  RENAME
  SUBSCRIBE
  UNSUBSCREBE
  LIST
  LSUB
  STATUS
  APPEND
  UID
  CHECK
  CLOSE
  EXPUNGE
  SEARCH
  FETCH
  STORE
  COPY
  -----------------------------------------------------------------------

  [ご注意]
  本バージョンは試用有効期間を設けたベータ版です。
　試用有効期間、インストールし、最初の起動から１８０日間です。
  解除キーを入れず１８０日間を過ぎると受信機能そのものが停止します。
  ----------------------------------------------------------------------
　試用有効期間が過ぎる頃には多分、正式リリースされていると思います。
  ----------------------------------------------------------------------

-------------------------------------------------------------------------
[使用環境]
-------------------------------------------------------------------------
Windows NT4.0/2000/XPにて、SPA-MS(Pro)を利用しているマシン。

-------------------------------------------------------------------------
[インストール]
-------------------------------------------------------------------------
１．システムには、ADMINISTRATOR権限でログインしてください。
２．SPA-IMAP4S.EXEの保存フォルダに移動し以下の命令を入力します。
    例）
       C:\>CD "Program Files\SPA-Pro"
       C:\Program Files\SPA-Pro>spa-imap4s -install
       SPA-Pro IMAP4rev1 Server installed.

    以上で、サービスとしてインストールされました。

-------------------------------------------------------------------------
[DOSプロンプトからの解除キーの登録方法]
-------------------------------------------------------------------------
１．DOSプロンプトにて、SPA-IMAP4S.EXE の保存フォルダに移動し以下の通り実行します。
    SPA-IMAP4Sが動作中のままで登録できます。
　  注意）SPA-IMAP4Sをインストールしてから行ってください。

    例）
       C:>CD "Program Files\SPA-Pro"
       C:\Program Files\SPA-Pro>spa-imap4s -key <解除キー値> <- ここを入力。
       It succeeded.                     　                 <- 登録されると応答が返ります。

-------------------------------------------------------------------------
[アンインストール]
-------------------------------------------------------------------------
１．メール受信サービス "SPA-Pro IMAP4rev1 Server"を停止します。
    [コントロールパネル]
      ->[サービス]
        ->[SPA-Pro IMAP4rev1 Server]
          ->”動作停止”に変更

２．DOSプロンプトにて、SPA-IMAP4.EXE の保存フォルダに移動し以下の通り実行します。
    例）
       C:>CD "Program Files\SPA-Pro"
       C:\Program Files\SPA-Pro>spa-imap4s -remove
       SPA-Pro IMAP4 Server removed.

    以上でサービスからアンインストールされました。

-------------------------------------------------------------------------
[バージョンの確認]
-------------------------------------------------------------------------
１．DOSプロンプトにて、SPA-IMAP4S.EXE の保存フォルダに移動し以下の通り実行します。
    例）
       C:>CD "Program Files\SPA-Pro"
       C:\Program Files\SPA-Pro>spa-imap4s -version
       SPA-Pro IMAP4rev1 Server Beta(0.01)  Copyright (C) 2002 Kawakami, Katsuyuki.

-------------------------------------------------------------------------
[アップデートについて]
-------------------------------------------------------------------------
プログラムの差替えは、「サービス停止」させた後、プログラムを新しいものに
コピーして「サービス起動」すれば、差替えは完了します。

この際の解除キーの再入力は基本的に必要ありません。

但し、サービスの「アンインストール」をした場合は再入力の必要があります。

-------------------------------------------------------------------------
[デバッグモードでの動作確認]
-------------------------------------------------------------------------
動作に対するお問い合わせは、以下のデータをテキストなどにトレースし添付して
送って頂けると問題解決の早道になります。

１．DOSプロンプトにて、SPA-IMAP4.EXE の保存フォルダに移動し以下の通り実行します。
    サービスは停止させておく必要があります。
    例）
       C:\>CD "Program Files\SPA-Pro"
       C:\Program Files\SPA-Pro>spa-imap4s -debug
       Debugging SPA-Pro IMAP4rev1 Server.
       ---- regestry ----
       Priority            0
       User Manager        SPA use
       Account folder      I:\Program Files\SPA-Pro\
       Host IP version     IPv4
       Host Name(IPv6)
       Accept limit        0(Unlimited)
       Timer               300(sec)
       Send Data Timer     on 60(s)
       Imap4 IP            permit all
       Imap4 Port          143
       MailGroup           "IMSUsers"
       MailSpoolDir        E:\IMS\
       MailBoxDir          E:\IMS\INBOX\%USERNAME%
       MailDataExtension   *.MSG
       Program Path
       Imap Password File  apop.dat
       AcceptImap4 Log     off
       Imap4Log            off
       -------------------
       SPA-Pro IMAP4rev1 Server Beta(0.01) trial period limiting on the 180 days.  Thu,
        21 Nov 2002 12:27:17 +0900
       windows 5.0 Build 2195 (Service Pack 3)
       Intel 1 processor in the system.
       host.domain=[xxx.xx.xx]
       wait select()

    以上のままにしておきメール受信の動作をトレースし確認ができます。
    テキストにトレース保存する場合は、
    C:\Program Files\SPA-Pro>SPA-IMAP4S -debug > log.txt
    など、パイプを使って任意のテキストにトレースさせてください。

    停止するときは、[CTRL-C]で停止します。

-------------------------------------------------------------------------
[設定レジストリ]

[imap4ログ設定] 　　
 HKEY_LOCAL_MACHINE
  ->SYSTEM
    ->CurrentControlSet
      ->Services
        ->SPAIMAP4S-PRO
          ->Parameters
            +->Imap4LogEnabled 0:しない,1:する

[imap4詳細ログ設定]
 HKEY_LOCAL_MACHINE
  ->SYSTEM
    ->CurrentControlSet
      ->Services
        ->SPAIMAP4S-PRO
          +->AcceptlogEnabled　0:しない,1:する
          　（"receiveimap4"フォルダは手動で作成して下さい。）

[POP3からは接続しない]
HKEY_LOCAL_MACHINE
 ->SOFTWARE
   ->EMWAC
     ->IMS
       +->POP3IsNotUsed 1:接続しない, 0:接続する