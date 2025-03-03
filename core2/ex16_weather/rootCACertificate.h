const char* rootCACertificate = \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDXzCCAkegAwIBAgILBAAAAAABIVhTCKIwDQYJKoZIhvcNAQELBQAwTDEgMB4G\n"\
"A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjMxEzARBgNVBAoTCkdsb2JhbFNp\n"\
"Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDkwMzE4MTAwMDAwWhcNMjkwMzE4\n"\
"MTAwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMzETMBEG\n"\
"A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n"\
"hvcNAQEBBQADggEPADCCAQoCggEBAMwldpB5BngiFvXAg7aEyiie/QV2EcWtiHL8\n"\
"RgJDx7KKnQRfJMsuS+FggkbhUqsMgUdwbN1k0ev1LKMPgj0MK66X17YUhhB5uzsT\n"\
"gHeMCOFJ0mpiLx9e+pZo34knlTifBtc+ycsmWQ1z3rDI6SYOgxXG71uL0gRgykmm\n"\
"KPZpO/bLyCiR5Z2KYVc3rHQU3HTgOu5yLy6c+9C7v/U9AOEGM+iCK65TpjoWc4zd\n"\
"QQ4gOsC0p6Hpsk+QLjJg6VfLuQSSaGjlOCZgdbKfd/+RFO+uIEn8rUAVSNECMWEZ\n"\
"XriX7613t2Saer9fwRPvm2L7DWzgVGkWqQPabumDk3F2xmmFghcCAwEAAaNCMEAw\n"\
"DgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFI/wS3+o\n"\
"LkUkrk1Q+mOai97i3Ru8MA0GCSqGSIb3DQEBCwUAA4IBAQBLQNvAUKr+yAzv95ZU\n"\
"RUm7lgAJQayzE4aGKAczymvmdLm6AC2upArT9fHxD4q/c2dKg8dEe3jgr25sbwMp\n"\
"jjM5RcOO5LlXbKr8EpbsU8Yt5CRsuZRj+9xTaGdWPoO4zzUhw8lo/s7awlOqzJCK\n"\
"6fBdRoyV3XpYKBovHd7NADdBj+1EbddTKJd+82cEHhXXipa0095MJ6RMG3NzdvQX\n"\
"mcIfeg7jLQitChws/zyrVQ4PkX4268NXSb7hLi18YIvDQVETI53O9zJrlAGomecs\n"\
"Mx86OyXShkDOOyyGeMlhLxS67ttVb9+E7gUJTb0o2HLO02JQZR7rkpeDMdmztcpH\n"\
"WD9f\n"\
"-----END CERTIFICATE-----\n";

// 証明書はブラウザ機能で容易に取得できる。
// ①HTTPSで対象サイトにアクセス
// ②ブラウザのURL入力欄の錠前アイコンから「接続がセキュリティで保護されています」を選択し、
// 　ポップ画面の右上の証明書アイコンをクリックすると「証明書ビューア」が開く。
// ③詳細に切り替えると「証明書の階層」が表示されるので、取得したい証明書を選択してから
// 　エクスポートで出力できる。
// 　今回は最も上位のルートCA証明書を選んだ。(他を選んでも、同サイトの証明書は得られる)
//
// Base64変換はWindowsの機能で実施する。
// ①上記で保存した証明書をWindowsで開く
// ②「詳細」タブ→「ファイルにコピー」→「Base 64」を選択して保存する。

/*
	証明書を確認するとき（安全）：
    client.setCACert(rootCACertificate);    // ルートCA証明書を設定
    
    証明書を確認しないとき（有効期限切れ時などの動作確認用）：
    client.setInsecure();                   // サーバ証明書を確認しない
*/

/*
取得時先URL：
https://www.jma.go.jp/bosai/forecast/data/forecast/130000.json

更新ファイル：
rootCACertificate.h

使用箇所：
\m5\core\ex16_weather
\m5\core\ex16_weather_basic
\m5\core2\ex16_weather

*/