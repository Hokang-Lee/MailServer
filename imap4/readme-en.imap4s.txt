------------------------------------------------------------------
SPA POP3 Server
------------------------------------------------------------------
[Soft name]                 SPA POP3 Server
[Registration The name]     SPAPOP111.ZIP
[Copyright The person]      Katuyuki Kawakami (kawa@mps.ne.jp)
[Carrying The person]       Copyright person and the same
[Correspondence The model]  Windows NT4.0/Windows 2000
[Software classification]   The shareware
[Reprinting The condition]  Give contact beforehand.
[Soft explanation]          "POP3 Server" not to compromise the environment to 
                            have been operating by replacing a difference and to
                            be connected with "SPA SMTP Server" which it is possible
                            to shift with "POP3S" of EMWAC IMS (the mail service) It is.

- It prohibits the quotation and the reprinting of the contents without the permission. -
-------------------------------------------------------------------------
[Introduction]
-------------------------------------------------------------------------
  EMWAC IMS was the free mail service program which works on NT, but the
  number of the users, too, can do the mail service which was stable with the
  simple setting without the limiting and it was extremely convenient for it.

  "POP3"Server not to compromise the environment to have been operating by
  replacing a difference and to be connected with SPA SMTP Server which it is
  possible to shift with "POP3S" of EMWAC IMS (the mail service) It is.

  SPA POP3 Server corresponds to the following command.
-------------------------------------------------------------------------
[POP3 The Server command]
-------------------------------------------------------------------------
POP3 Server is a RFC It executes the POP3 protocol which is defined about
1460.
The POP3 command which is supported in this version :
-------------------------------------------------------------------------
USER name
PASS string
STAT
LIST [msg]
RETR msg
DELE msg
NOOP
TOP msg n
LAST
RSET
QUIT
UIDL [msg]
-------------------------------------------------------------------------
APOP name digest <-The authentication becomes not made when passing and not
putting a cancellation key in the 30th.
-------------------------------------------------------------------------
[When making APOP available]
-------------------------------------------------------------------------
To make APOP available, it establishes the following setting every account in the mailbox folder.

  1.It makes a text file newly.
    "<mailbox folder>\<object account>\APOP.DAT"

  2. It opens the above file "APOP.DAT" with the memo book and so on, write
     the password which corresponded to each mailbox account into this file
     and preserve it.

  3. It makes the authentication of the using mail client APOP.
     Later, receive a mail ordinarily.

Attention )
Be careful because the APOP authentication fails when there is a mistake in
the password by which the above file didn't exist and it set.

-------------------------------------------------------------------------
[Environment]
-------------------------------------------------------------------------
The machine that the mail server which used EMWAC IMS Windows NT4.0 since then
is operating
Be careful because this program is only for the Intel edition.

-------------------------------------------------------------------------
[File composition]
-------------------------------------------------------------------------
 The following two files are born in "SPAP3Sxxx.ZIP".

   SPAPOP3S.EXE        (Program body)
   README.SPAPO3S.TXT (This file)

-------------------------------------------------------------------------
[Condition of the use]
-------------------------------------------------------------------------
 1) We make the present version of this software a shareware.
 2) When reprinting this software, we request a mail to the author beforehand.
 3) We request to add reverse assembly and reorganization to this software not to be.
 4) The author doesn't bear responsibility about the damage which occurs directly or
    indirectly about the use of this software.

    Unless agreeing to above condition, the use of this software be reserved.

-------------------------------------------------------------------------
[About the way of remitting and the cancellation key]
-------------------------------------------------------------------------
We make this program 30 days behind the installation in the trial period.
In case of use its since then, to use as it doesn't set a cancellation key
gets for APOP authentication not to be made.
When wanting to continue continuously including the APOP authentication, remit
3,000 yen until the following directing.

Also, when installing [SPA POP3 Server] in more than one machine and using it,
remit the amount of money of 3,000-yen × [SPA POP3 the number of the stands
to install Server in].
Incidentally, we suppose only that the method of payment is bank 振込.

-------------------------------------------------------------------------
[The way of remitting]
-------------------------------------------------------------------------
  1) The bank remittance seeing place
     We ask the following account for the remittance.
      Sumitomo Bank           The spring day part branch (Floor walker No. 903)
      The account number      607887
      The account kind        The ordinary deposit
      The account name        アクセス ダイヒョウ カワカミ カツユキ

* Report the effect which transferred after remittance with the mail for I 
  (kawa@mps.ne.jp).
  Also, always write the following at the mail.

  (1) The soft name : SPA POP3 Server
  (2) The number of the licenses
  (3) The remitter name (The name which was written in the place of the remitter name in bank remittance)
  (4) The answer place E-mail address
  (5) The remittance date

It sends a cancellation key with the mail as soon as it is possible to confirm that it was remitted.

-------------------------------------------------------------------------
[Installation]
-------------------------------------------------------------------------
To install EMWAC IMS and that sending and receiving is normally made are a presupposition.

1. There is a log in in the ADMINISTRATOR authority.

2. It thaws, it preserves SPAPOP3S.ZIP in the preservation folder of EMWAC IMS and it adds it to the service.
    ex.)
       C:>CD imsi386 
       C:\imsi386>spapop-install

3. The mail receiving service It stops. "IMS POP3 Receiver"
    [Control Panel]
      - > [The service]
        ->[IMS POP3 Server]
          -> It changes into "stop" & "manual".
    

5. The mail receiving service It starts. "SPA POP3 Server"
    [Control Panel]
      - > [The service]
        ->[SPA POP3 Server]
          -> It changes into "start" & "automatic".

6. To confirm operation, in Telenet, connect with 3 S of SPAPOP as follows and attempt to confirm.

    telnet <mail server> pop3<cr>        <---- It inputs this place.
    ------------------------------------------
    S:+OK SPA POP3 Server (1.xx) Ready <xxxxxx.xxxxxxxxx@xxx.xxxx.co.jp>
    C:USER <username><cr>                <---- It inputs this place.
    S:+OK <username> is welcome here
    C:PASS <password><cr>
    S:+OK <uername>'s mailbox has x message(x) (xxxxx octet) Or
    S:-ERR <usename> logon failed(1326) in PASS
    ------------------------------------------

   As mentioned above, it becomes available if doing user authentication and the
   replying that James Grover Thurber opens a mailbox is done.
   "-ERR If " comes out, there are account setting and possibility that a
   password, a local group aren't set right.

-------------------------------------------------------------------------
[About the update]
-------------------------------------------------------------------------
The difference replacement completes if copying a program in the new one about the
difference replacement by the program after made to be "the service stop" and
being "the service starting".

There is not re-input of the cancellation key in now basically, being necessary.

But, when doing "Un installation" of the service, there is re-input's necessity.

-------------------------------------------------------------------------
[Un installation]
-------------------------------------------------------------------------
1. The mail receiving service It stops. "SPA POP3 Server"
    [Control Panel]
      -> [The service]
        ->[SPA POP3 Server]
          -> It changes into. "operation stop "

2. In the DOS prompt, it moves to the preservation folder of SPAPOP3S.EXE and it executes as follows.
    ex. )
       C:>CD imsi386
       C:\imsi386>spapop-remove
       C:\imsi386>SPA POP3 Server removed.
       C:\imsi386>
    Un installation completed above.

-------------------------------------------------------------------------
[Confirmation of the version]
-------------------------------------------------------------------------
1. In the DOS prompt, it moves to the preservation folder of SPAPOP3S.EXE and it executes as follows.
    ex. )
       C:>CD imsi386
       C:\imsi386>spapop-version
       C:\imsi386>SPA POP3 Server (1.xx) Mon, 06 Sep 1999 12:33:11 +0900 
       C:\imsi386>
    The version of this program can be confirmed above.

-------------------------------------------------------------------------
[Operation confirmation by the debug mode]
-------------------------------------------------------------------------
The inquiry of the operation becomes the shortcut of the problem solving when
tracing out and it is possible to send the following data to the text and so on,
attaching.

1. In the DOS prompt, it moves to the preservation folder of SPAPOP3S.EXE and it executes as follows.
    It is necessary to make service stop beforehand.
    ex. )
       C:>CD imsi386
       C:\imsi386>spapop-debug

       Debugging SPA POP3 Server.
       ---- regestry ----
       Pop3 Port 110
       MailGroup "IMSUsers"
       MailBoxDir E:\IMS\INBOX\%USERNAME%
       MailSpoolDir E:\IMS\
       Program Path E:\imsi386\
       Pop3Log on
       -------------------
       SPA POP3 Server (1.00) Wed, 17 May 2000 11:58:22 +0900
       windows 4.0 Build 1381 (Service Pack 6)
       Intel 1 processor in the system.

    It is beforehand as above, it traces out mail receiving operation and it is
    possible to do confirmation.
    When preserving a trace at the text

    C:\imsi386>spapop3s -debug > log.txt

    Make an optional text trace out using the pipe.
    When stopping, it stops at [CTRL-C].

-------------------------------------------------------------------------
[Others]
-------------------------------------------------------------------------
We request an opinion and so on in E-mail:kawa@mps.ne.jp (Katuyuki Kawakami).
Also, it plans to do an up-to-the-minute information in
"http://www.mps.ne.jp/personal/kawa/".
-------------------------------------------------------------------------
[Change record]
-------------------------------------------------------------------------
v1.00 2000.05.18 spapop3s.exe It releases as the test edition.

v1.01 2000.05.21 1. The defect where the folder of the local user isn't automatically made
                 2. The defect which the folder of the local user isn't opened to after logging out behind the user authentication, too

v1.02 2000.05.25 1. The defect which does a hang when doing mail receiving with big size

v1.03 2000.05.27 1. The defect which can not be received with Eudora

v1.04 2000.06.09 When it is possible for 1.IMS to use non-installation
                 2. The dealing with APOP
                 3. The defect of the possibility that there is a hang with the condition
                 4. The closing forgetting of a handle
                 5. The defect of the possibility that there is a hang in the LIST, UIDL command with the timing

v1.05 2000.06.17 1. When it is possible to use %HOME% variable to the mailbox folder
                 2. It changes the default of the work folder into "%SystemRoot%\SYSTEM32\SPA\MAIL".
                 3. It changes the default of the mailbox folder into "%HOME%\INETMAIL\INBOX".
                 4. It changes the default of the manager address into "Administrator".

v1.06 2000.06.24 1. The defect whic  isn't replaced with the default when the setting of a work folder, a mailbox folder, a manager address is a blank
                 2. The defect where there is possibility that there is a hang when stopping before replying after client connection

v1.07 2000.06.24 The defect which the RETR command fails in after issuing 1.TOP command

v1.08 2000.07.20 1. By the domain name of the default for the notion of the mechanical address under the environment setting that the object machine can not do an address draw The defect which isn't written

v1.09 2000.08.10 1. The defect which doesn't open a mailbox according to the account when there is not %home% or environment variable %username% specification in the specification of the mailbox folder

v1.10 2000.09.04 1. Correcting as it becomes accessible 5 minutes later with the default when the access to the mailbox becomes an overlap access condition for some reason
                 2. The defect where the object mail data becomes a forwarding error in reconnection when communication is cut compulsory while forwarding data a mail

v1.11 2000.09.20 1. The defect where there is possibility to lock in mail data transmission with the big environment of the traffic
-------------------------------------------------------------------------
